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
	// �����е� socketDescriptor �� QTcpSocket ����
	if (!m_socket.setSocketDescriptor(socketDescriptor)) {
		LogWidget::instance()->addLog(QString("Failed to set socket descriptor: %1").arg(socketDescriptor), LogWidget::Warning);
		return false;
	}
	// �������ݵ���ͳ�ʱ���źŲ�
	connect(&m_socket, &QTcpSocket::readyRead, this, &ConnectionHandler::onReadyRead);
	connect(&m_socket, &QTcpSocket::disconnected, this, &ConnectionHandler::disconnectFromPeer);
	connect(&m_timer, &QTimer::timeout, this, &ConnectionHandler::onTimeout);
	LogWidget::instance()->addLog(QString("Starting connection handler for %1").arg(m_socket.peerAddress().toString()), LogWidget::Info);
	return true;
}

void ConnectionHandler::pairWith(std::shared_ptr<ConnectionHandler> peer)
{
	m_peer = peer;
	// ��Գɹ���ֹͣ��ʱ��ʱ��
	m_timer.stop();
	// Ϊ����ת�������źŲۣ��������յ�����ʱת�����Զ�
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
	// �Ͽ������� m_socket ��ص��ź����ӣ���ֹ�������� onReadyRead �Ȳ�
	m_socket.disconnect();

	// ������ֹ���ӣ������������������������������
	m_socket.abort();

	// �������ԵĶԶ����ӣ�����������Ͽ�����ֹ��ѭ������� m_isDisconnecting ��ǣ�
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

	// �����û��ԣ���û�������֣������ȳ��Խ�������
	if (!m_peer) {
		RendezvousMessage msg;
		// ���Խ��������ݵ���һ����Ϣ������
		if (msg.ParseFromArray(data.constData(), data.size())) {
			if (msg.has_request_relay()) {
				RequestRelay requestRelay = msg.request_relay();
				QString uuid = QString::fromStdString(requestRelay.uuid());
				if (uuid.isEmpty()) {
					LogWidget::instance()->addLog("Received empty UUID in RequestRelay", LogWidget::Error);
					return;
				}
				// ֪ͨ�ⲿ�����������м̡������ⲿȥ���� pairWith() �������߼�
				emit relayRequestReceived(uuid);
				// --- ���������־���ɹ��� ---
				// ����������ֶ����������ݣ��Ǿ͵���һ�� onReadyRead() �ٴ���
				return;
			}
			else {
				LogWidget::instance()->addLog("Unknown or unexpected RendezvousMessage type", LogWidget::Warning);
				return;
			}
		}
		else {
			// �޷������� RendezvousMessage��˵���ⲻ�������ںϷ�����
			//LogWidget::instance()->addLog("Failed to parse RendezvousMessage from data (handshake stage)", LogWidget::Warning);
			return;
		}
	}
	else {
		// ����ԣ�����ֱ��ת��
		if (m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
			m_peer->socket()->write(data);
		}
	}
}
