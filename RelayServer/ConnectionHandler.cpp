#include "connectionhandler.h"
#include "LogWidget.h"

ConnectionHandler::ConnectionHandler(QObject* parent)
	: QObject(parent)
{

}

ConnectionHandler::~ConnectionHandler()
{
	disconnectFromPeer();
	if (m_socket.state() != QAbstractSocket::UnconnectedState) {
		m_socket.disconnectFromHost();
		m_socket.waitForDisconnected(3000);
	}
}

bool ConnectionHandler::start(qintptr socketDescriptor)
{
	// 将已有的 socketDescriptor 与 QTcpSocket 关联
	if (!m_socket.setSocketDescriptor(socketDescriptor)) {
		LogWidget::instance()->addLog(QString("Failed to set socket descriptor: %1").arg(socketDescriptor), LogWidget::Warning);
		return false;
	}
	// 连接数据到达和超时的信号槽
	connect(&m_socket, &QTcpSocket::readyRead, this, &ConnectionHandler::onReadyRead);
	connect(&m_socket, &QTcpSocket::disconnected, this, &ConnectionHandler::disconnectFromPeer);
	connect(&m_timer, &QTimer::timeout, this, &ConnectionHandler::onTimeout);
	LogWidget::instance()->addLog(QString("Starting connection handler for %1").arg(m_socket.peerAddress().toString()), LogWidget::Info);
	return true;
}

void ConnectionHandler::pairWith(std::shared_ptr<ConnectionHandler> peer)
{
	m_peer = peer;
	// 配对成功后停止超时定时器
	m_timer.stop();
	// 为数据转发建立信号槽：当本端收到数据时转发给对端
	connect(&m_socket, &QTcpSocket::readyRead, this, [this]() {
		QByteArray data = m_socket.readAll();
		if (m_peer && m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
			m_peer->socket()->write(data);
		}
		});
}

void ConnectionHandler::startTimeout(int msec)
{
	m_timer.start(msec);
}

void ConnectionHandler::onTimeout()
{
	LogWidget::instance()->addLog(QString("Connection timed out: %1").arg(m_socket.peerAddress().toString()), LogWidget::Warning);
	disconnectFromPeer();
}

void ConnectionHandler::disconnectFromPeer()
{
	if (m_isDisconnecting)
		return;
	m_isDisconnecting = true;

	m_timer.stop();
	// 断开所有与 m_socket 相关的信号连接，防止后续触发 onReadyRead 等槽
	m_socket.disconnect();

	// 立即中止连接，清除缓冲区，避免后续处理残留数据
	m_socket.abort();

	// 如果有配对的对端连接，则主动让其断开（防止死循环，检查 m_isDisconnecting 标记）
	if (m_peer) 
	{
		if (!m_peer->m_isDisconnecting) {
			m_peer->disconnectFromPeer();
		}
		m_peer.reset();
	}
	LogWidget::instance()->addLog(QString("Disconnected: %1").arg(m_socket.peerAddress().toString()), LogWidget::Info);
}

QTcpSocket* ConnectionHandler::socket()
{
	return &m_socket;
}



void ConnectionHandler::onReadyRead()
{
	if (m_isDisconnecting)
		return;
	QByteArray data = m_socket.readAll();

	// 如果还没配对（还没处理握手），就先尝试解析握手
	if (!m_peer) {
		RendezvousMessage msg;
		// 尝试将所有数据当作一个消息来解析
		if (msg.ParseFromArray(data.constData(), data.size())) {
			if (msg.has_request_relay()) {
				RequestRelay requestRelay = msg.request_relay();
				QString uuid = QString::fromStdString(requestRelay.uuid());
				if (uuid.isEmpty()) {
					LogWidget::instance()->addLog("Received empty UUID in RequestRelay", LogWidget::Error);
					return;
				}
				// 通知外部“有人请求中继”，由外部去调用 pairWith() 或其它逻辑
				emit relayRequestReceived(uuid);
				// --- 到这里握手就算成功了 ---
				// 如果接下来又读到其他数据，那就等下一次 onReadyRead() 再处理
				return;
			}
			else {
				LogWidget::instance()->addLog("Unknown or unexpected RendezvousMessage type", LogWidget::Warning);
				return;
			}
		}
		else {
			// 无法解析成 RendezvousMessage；说明这不是握手期合法数据
			//LogWidget::instance()->addLog("Failed to parse RendezvousMessage from data (handshake stage)", LogWidget::Warning);
			return;
		}
	}
	else {
		// 已配对，后续直接转发
		if (m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
			m_peer->socket()->write(data);
		}
	}
}
