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
	m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	// �������ݵ���ͳ�ʱ���źŲ�
	connect(&m_socket, &QTcpSocket::readyRead, this, &ConnectionHandler::onReadyRead);
	connect(&m_socket, &QTcpSocket::disconnected, this, &ConnectionHandler::disconnectFromPeer);
	connect(&m_timer, &QTimer::timeout, this, &ConnectionHandler::onTimeout);
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
	LogWidget::instance()->addLog(QString("Connection timed out: %1, from %2").arg(m_socket.peerAddress().toString()).arg(m_roleStr), LogWidget::Warning);
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
	LogWidget::instance()->addLog(QString("Disconnected: %1 from %2").arg(m_socket.peerAddress().toString()).arg(m_roleStr), LogWidget::Info);
}

QTcpSocket* ConnectionHandler::socket()
{
	return &m_socket;
}



void ConnectionHandler::onReadyRead()
{
	QByteArray buffer = m_socket.property("buffer").toByteArray();
	buffer.append(m_socket.readAll());

	while (buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		if (buffer.size() < 4 + static_cast<int>(packetSize))
			break;

		QByteArray packetData = buffer.mid(4, packetSize);
		QByteArray fullPacket;
		{
			quint32 bigEndianSize = qToBigEndian(packetSize);
			fullPacket.append(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));
			fullPacket.append(packetData);
		}

		buffer.remove(0, 4 + packetSize);

		// �����δ������֣���δ��ԣ��������������Ϣ����
		if (!m_peer) {
			RendezvousMessage msg;
			if (msg.ParseFromArray(packetData.constData(), packetData.size())) {
				if (msg.has_request_relay()) {
					RequestRelay requestRelay = msg.request_relay();
					QString uuid = QString::fromStdString(requestRelay.uuid());
					switch (requestRelay.role()) {
					case RequestRelay::DESK_CONTROL:
						m_roleStr = "DeskControl";
						break;
					case RequestRelay::DESK_SERVER:
						m_roleStr = "DeskServer";
						break;
					default:
						m_roleStr = "Unknown";
						break;
					}
					LogWidget::instance()->addLog(QString("Received RequestRelay from %1, UUID: %2")
						.arg(m_roleStr).arg(uuid), LogWidget::Info);
					emit relayRequestReceived(uuid);
				}
			}
			else {
				LogWidget::instance()->addLog("Failed to parse handshake message", LogWidget::Warning);
			}
		}
		else {
			// ����Ѿ���ԣ�ֱ�ӽ���������Ϣ��������ͷ��ת�����Զ�
			if (m_peer->socket()->state() == QAbstractSocket::ConnectedState) {
				m_peer->socket()->write(fullPacket);
			}
		}
	}
	// ��ʣ������ݴ�� socket �����У����´ζ�ȡʹ��
	m_socket.setProperty("buffer", buffer);
}

