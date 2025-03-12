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

// �ڷ��Ͷ� ��д����Ϣ�ĳ��ȣ������� QDataStream д�� 4 �ֽڵ�������ʹ�������ֽ��򣩣���д��ʵ�ʵ� protobuf ��Ϣ����
void ConnectionHandler::onReadyRead()
{
	QByteArray data = m_socket.readAll();
	processData(data);
}


void ConnectionHandler::processData(const QByteArray& data)
{

	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.constData(), data.size())) {
		LogWidget::instance()->addLog("Failed to parse RendezvousMessage from data", LogWidget::Warning);
		return;
	}
	// ���� oneof �ֶ��ж���Ϣ����
	if (msg.has_request_relay()) {
		// �����û����ԣ���Ϊ�յ������������ݣ���������ֱ��ת����
		if (!m_peer)
		{
			RequestRelay requestRelay = msg.request_relay();
			QString uuid = QString::fromStdString(requestRelay.uuid());
			if (uuid.isEmpty()) {
				LogWidget::instance()->addLog("Received relay request with empty UUID", LogWidget::Error);
				return;
			}
			emit relayRequestReceived(uuid);

		}
		else
		{
			// ����Ժ��յ�������ֱ��ת�����Զ�
			if (m_peer && m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
				m_peer->socket()->write(data);
			}
		}
	}
	else {
		LogWidget::instance()->addLog("Unknown message type in RendezvousMessage", LogWidget::Warning);
	}
}
