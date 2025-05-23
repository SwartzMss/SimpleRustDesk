#include "PeerClient.h"
#include "LogWidget.h"
#include <QUuid>
#include <QUrl>
#include <QtNetwork/QHostInfo>

PeerClient::PeerClient(const QString& uuid,QObject* parent)
	: QObject(parent), m_socket(nullptr), m_serverPort(0), m_connected(false), m_isRelayOnline(false)
{
	m_uuid = uuid;
	m_reconnectTimer = new QTimer(this);
	m_reconnectTimer->setInterval(3000);
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
       if (m_relayManager) {
        m_relayManager->stop();
        m_relayManager->deleteLater();
        m_relayManager = nullptr;
    }
}

void PeerClient::onConnected()
{
	m_connected = true;
	m_reconnectTimer->stop();
	LogWidget::instance()->addLog("Connected successfully", LogWidget::Info);

	RegisterPeer regPeer;
	regPeer.set_uuid(m_uuid.toStdString());

	RendezvousMessage msg;
	*msg.mutable_register_peer() = regPeer;

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		emit errorOccurred("Serialization failed");
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));

	quint32 packetSize = static_cast<quint32>(data.size());
	quint32 bigEndianSize = qToBigEndian(packetSize);
	QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

	QByteArray fullData;
	fullData.append(header);
	fullData.append(data);

	m_socket->write(fullData);
	m_socket->flush();
	LogWidget::instance()->addLog(QString("Sent RegisterPeer message with uuid %1").arg(m_uuid), LogWidget::Info);
}

void PeerClient::onReadyRead()
{
	m_buffer.append(m_socket->readAll());

	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		if (m_buffer.size() < 4 + static_cast<int>(packetSize))
			break;

		QByteArray data = m_buffer.mid(4, packetSize);

		m_buffer.remove(0, 4 + packetSize);
		RendezvousMessage msg;
		if (!msg.ParseFromArray(data.data(), data.size())) {
			emit errorOccurred("Failed to parse RendezvousMessage");
			return;
		}

		if (msg.has_register_peer_response()) {
			RegisterPeerResponse response = msg.register_peer_response();
			if (response.result() == Result::OK) {
				emit registrationResult(Result::OK);
			}
			else if (response.result() == Result::INNER_ERROR) {
				emit registrationResult(Result::INNER_ERROR);
			}
			else {
				emit registrationResult(response.result());
			}
		}
		else if (msg.has_punch_hole()) {
			// 收到来自 TCP 的 PunchHole 消息
			LogWidget::instance()->addLog("Received PunchHole message from server", LogWidget::Info);

			// 构造 PunchHoleSent 消息，填充所需字段（这里的 relay_server 和 relay_port 可根据实际情况设置）
			PunchHoleSent sent;
			sent.set_id(msg.punch_hole().id());
			if (!m_isRelayOnline)
			{
				sent.set_result(Result::RELAYSERVER_OFFLINE);
			}
			else
			{
				sent.set_relay_server(m_relayIP.toStdString());
				sent.set_relay_port(m_relayPort);
				sent.set_result(Result::OK);
			}

			RendezvousMessage reply;
			*reply.mutable_punch_hole_sent() = sent;

			std::string outStr;
			if (!reply.SerializeToString(&outStr)) {
				emit errorOccurred("Failed to serialize PunchHoleSent message");
				return;
			}
			QByteArray data(outStr.data(), static_cast<int>(outStr.size()));

			quint32 packetSize = static_cast<quint32>(data.size());
			quint32 bigEndianSize = qToBigEndian(packetSize);
			QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

			QByteArray fullData;
			fullData.append(header);
			fullData.append(data);

			m_socket->write(fullData);
			m_socket->flush();
			LogWidget::instance()->addLog("Sent PunchHoleSent message in response", LogWidget::Info);

			// If the result is OK, start the RelayManager to establish a TCP connection to the relay server.
			if (sent.result() == Result::OK) {
				if (!m_relayManager) {
					m_relayManager = new RelayManager(this);
				}
				QUrl relayUrl = QUrl::fromUserInput(m_relayIP);
				QString relayHost = relayUrl.host().isEmpty() ? m_relayIP : relayUrl.host();

				QHostAddress resolvedRelayAddress;
				if (!resolvedRelayAddress.setAddress(relayHost)) {
					// 如果直接转换失败，则尝试 DNS 解析
					QHostInfo info = QHostInfo::fromName(relayHost);
					if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
						LogWidget::instance()->addLog("Failed to resolve Relay IP: " + relayHost, LogWidget::Error);
						return;
					}
					// 遍历地址列表，筛选 IPv4 地址
					bool foundIPv4 = false;
					for (const QHostAddress& address : info.addresses()) {
						if (address.protocol() == QAbstractSocket::IPv4Protocol) {
							resolvedRelayAddress = address;
							foundIPv4 = true;
							break;
						}
					}
					if (!foundIPv4) {
						LogWidget::instance()->addLog("No IPv4 address found for Relay IP: " + relayHost, LogWidget::Error);
						return;
					}
				}

				m_relayManager->start(resolvedRelayAddress, m_relayPort, m_uuid);
			}
		}
		else {
			LogWidget::instance()->addLog("Received unknown message type", LogWidget::Warning);
		}
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
	emit errorOccurred("Disconnected from server");

	if (!m_isStopping) {
		m_reconnectTimer->stop();
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
		m_socket = nullptr;
		LogWidget::instance()->addLog("Disconnected from server, attempting to reconnect...", LogWidget::Info);
		// 启动重连定时器，等待一段时间后尝试重新连接
		m_reconnectTimer->start();
	}
}

void PeerClient::attemptReconnect()
{
	if (!m_isStopping && !m_connected) {
		LogWidget::instance()->addLog("Attempting to reconnect...", LogWidget::Info);
		doConnect();
	}
}
