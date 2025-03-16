#include "RelayPeerClient.h"
#include "LogWidget.h"  // ʹ�� LogWidget ��¼��־
#include "rendezvous.pb.h"

RelayPeerClient::RelayPeerClient(QObject* parent)
	: QObject(parent),
	m_udpSocket(new QUdpSocket(this)),
	m_relayPort(0),
	m_heartbeatTimer(new QTimer(this))
{
	// �󶨶�ȡ���ݲ�
	connect(m_udpSocket, &QUdpSocket::readyRead, this, &RelayPeerClient::onReadyRead);
	// ���ö�ʱ��������ÿ 5 �뷢��һ�� heartbeat
	m_heartbeatTimer->setInterval(5000);
	connect(m_heartbeatTimer, &QTimer::timeout, this, &RelayPeerClient::sendHeartbeat);
}

RelayPeerClient::~RelayPeerClient()
{
	stop();
}

void RelayPeerClient::start(const QHostAddress& relayAddress, quint16 relayPort)
{
	m_isAlive = true;
	m_relayAddress = relayAddress;
	m_relayPort = relayPort;

	// �󶨵�������ö˿�
	if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
		emit errorOccurred("Failed to bind UDP socket");
		return;
	}
	// ��������һ�� Heartbeat
	sendHeartbeat();
	// ������ʱ��
	m_heartbeatTimer->start();
	LogWidget::instance()->addLog("Started RelayPeerClient heartbeat", LogWidget::Info);
}

void RelayPeerClient::stop()
{
	if (m_heartbeatTimer->isActive())
		m_heartbeatTimer->stop();
	m_udpSocket->close();
}

void RelayPeerClient::sendHeartbeat()
{
	if (m_isAlive == false)
	{
		emit errorOccurred("Heartbeat message not responsed");
	}
	m_isAlive = false;

	// ���� Heartbeat ��Ϣ
	RendezvousMessage msg;
	msg.mutable_heartbeat();  // ���� heartbeat �ֶ�

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		emit errorOccurred("Failed to serialize Heartbeat message");
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
	qint64 bytesSent = m_udpSocket->writeDatagram(data, m_relayAddress, m_relayPort);
	if (bytesSent == -1) {
		emit errorOccurred("Failed to send Heartbeat datagram");
	}
}

void RelayPeerClient::onReadyRead()
{
	while (m_udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(m_udpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;
		m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

		RendezvousMessage response;
		if (response.ParseFromArray(datagram.data(), datagram.size())) {
			if (response.has_heartbeat()) {
				m_isAlive = true;
				emit heartbeatResponseReceived();
			}
		}
		else {
			emit errorOccurred("Failed to parse Heartbeat response");
		}
	}
}
