#include "PeerClient.h"
#include "LogWidget.h"
#include <QUuid>

PeerClient::PeerClient(QObject* parent)
	: QObject(parent), m_socket(nullptr), m_serverPort(0), m_connected(false), m_isRelayOnline(false)
{
	m_reconnectTimer = new QTimer(this);
	m_reconnectTimer->setInterval(3000);  // ÿ 3 ������һ��
	m_reconnectTimer->setSingleShot(true);
	connect(m_reconnectTimer, &QTimer::timeout, this, &PeerClient::attemptReconnect);
}

PeerClient::~PeerClient()
{
	stop();
}

void PeerClient::setRelayStatus(bool isOnline)
{
	m_isRelayOnline = isOnline;
}

void PeerClient::setRelayInfo(const QString& ip, int port) {
	m_relayIP = ip;
	m_relayPort = port;
}

void PeerClient::doConnect()
{
	if (m_socket) {
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::connected, this, &PeerClient::onConnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &PeerClient::onReadyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &PeerClient::onDisconnected);
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
		this, SLOT(onSocketError(QAbstractSocket::SocketError)));

	LogWidget::instance()->addLog(QString("Trying to connect to %1:%2")
		.arg(m_serverAddress.toString()).arg(m_serverPort), LogWidget::Info);
	m_socket->connectToHost(m_serverAddress, m_serverPort);
	m_reconnectTimer->start();
}

void PeerClient::start(const QHostAddress& address, quint16 port)
{
	m_serverAddress = address;
	m_serverPort = port;
	m_isStopping = false;
	m_connected = false;

	doConnect();
}

void PeerClient::stop()
{
	m_isStopping = true;
	if (m_reconnectTimer->isActive())
		m_reconnectTimer->stop();
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
}

void PeerClient::onConnected()
{
	m_connected = true;
	m_reconnectTimer->stop();
	LogWidget::instance()->addLog("Connected successfully", LogWidget::Info);

	// ���� UUID �ַ��������� RegisterPeer ��Ϣ
	static QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
	RegisterPeer regPeer;
	regPeer.set_uuid(uuidStr.toStdString());

	RendezvousMessage msg;
	*msg.mutable_register_peer() = regPeer;

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		emit errorOccurred("Serialization failed");
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
	m_socket->write(data);
	m_socket->flush();
	LogWidget::instance()->addLog(QString("Sent RegisterPeer message with uuid %1").arg(uuidStr), LogWidget::Info);
}

void PeerClient::onReadyRead()
{
	QByteArray data = m_socket->readAll();
	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.data(), data.size())) {
		emit errorOccurred("Failed to parse RendezvousMessage");
		return;
	}

	if (msg.has_register_peer_response()) {
		// ԭ����ע�ᴦ���߼�
		RegisterPeerResponse response = msg.register_peer_response();
		if (response.result() == RegisterPeerResponse::OK) {
			emit registrationResult(RegisterPeerResponse::OK);
		}
		else if (response.result() == RegisterPeerResponse::SERVER_ERROR) {
			emit registrationResult(RegisterPeerResponse::SERVER_ERROR);
		}
		else {
			emit registrationResult(response.result());
		}
	}
	else if (msg.has_punch_hole()) {
		// �յ����� TCP �� PunchHole ��Ϣ
		LogWidget::instance()->addLog("Received PunchHole message from server", LogWidget::Info);

		// ���� PunchHoleSent ��Ϣ����������ֶΣ������ relay_server �� relay_port �ɸ���ʵ��������ã�
		PunchHoleSent sent;
		sent.set_id(msg.punch_hole().id());
		if (!m_isRelayOnline)
		{
			sent.set_result(PunchHoleSent::ERR);
		}
		else
		{
			sent.set_relay_server(m_relayIP.toStdString());
			sent.set_relay_port(m_relayPort);
			sent.set_result(PunchHoleSent::OK);
		}

		// �� PunchHoleSent ��ϢǶ�뵽 RendezvousMessage ��
		RendezvousMessage reply;
		*reply.mutable_punch_hole_sent() = sent;

		std::string outStr;
		if (!reply.SerializeToString(&outStr)) {
			emit errorOccurred("Failed to serialize PunchHoleSent message");
			return;
		}
		m_socket->write(outStr.data(), outStr.size());
		m_socket->flush();
		LogWidget::instance()->addLog("Sent PunchHoleSent message in response", LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog("Received unknown message type", LogWidget::Warning);
	}
}


void PeerClient::onSocketError(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error);
	if (!m_isStopping && !m_connected)		{
		m_reconnectTimer->start();
	}
	emit errorOccurred(m_socket->errorString());
}


void PeerClient::onDisconnected()
{
	m_connected = false;
	if (!m_isStopping) {
		// �Ͽ�����������
		m_reconnectTimer->start();
	}
	emit errorOccurred("Disconnected from server");
}

void PeerClient::attemptReconnect()
{
	if (!m_isStopping && !m_connected) {
		LogWidget::instance()->addLog("Attempting to reconnect...", LogWidget::Info);
		doConnect();
	}
}