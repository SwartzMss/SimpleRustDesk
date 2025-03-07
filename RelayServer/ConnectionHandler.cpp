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
	m_socket.disconnectFromHost();
}

void ConnectionHandler::disconnectFromPeer()
{
	if (m_socket.state() == QAbstractSocket::ConnectedState) {
		m_socket.disconnectFromHost();
		if (m_socket.state() != QAbstractSocket::UnconnectedState) {
			m_socket.waitForDisconnected(3000);
		}
	}
	if (m_peer) {
		m_peer->m_peer.reset();
		m_peer.reset();
	}
	LogWidget::instance()->addLog(QString("Disconnected: %1").arg(m_socket.peerAddress().toString()), LogWidget::Info);
}

QTcpSocket* ConnectionHandler::socket()
{
	return &m_socket;
}

// 在发送端 先写入消息的长度（例如用 QDataStream 写入 4 字节的整数，使用网络字节序），再写入实际的 protobuf 消息数据
void ConnectionHandler::onReadyRead()
{
	QByteArray data = m_socket.readAll();
	processData(data);
}


void ConnectionHandler::processData(const QByteArray& data)
{
	// 如果还没有配对，认为收到的是握手数据（后续数据直接转发）
	if (!m_peer) 
	{
		// 解析 proto 消息，注意这里假设一次能收到完整消息
		RequestRelay  request;
		if (!request.ParseFromArray(data.constData(), data.size())) {
			LogWidget::instance()->addLog("Failed to parse RelayRequest from proto data", LogWidget::Error);
			return;
		}
		QString id = QString::fromStdString(request.id());
		QString uuid = QString::fromStdString(request.uuid());

		if (uuid.isEmpty()) {
			LogWidget::instance()->addLog("Received relay request with empty UUID", LogWidget::Error);
			return;
		}

		// 根据 id 判断连接角色
		if (!id.isEmpty()) {
			// 发起方：先进行缓存等待对方连接
			LogWidget::instance()->addLog(QString("Received initiator relay request: id=%1, uuid=%2").arg(id, uuid), LogWidget::Info);
		}
		else {
			// 被控制方：等待与发起方进行配对
			LogWidget::instance()->addLog(QString("Received controlled relay request with uuid: %1").arg(uuid), LogWidget::Info);
		}

		// 发射信号，由 RelayServer 的 tryPairing 来完成配对（第一来缓存、第二来配对）
		emit relayRequestReceived(uuid);
	}
	else 
	{
		// 已配对后，收到的数据直接转发给对端
		if (m_peer && m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
			m_peer->socket()->write(data);
		}
	}
}
