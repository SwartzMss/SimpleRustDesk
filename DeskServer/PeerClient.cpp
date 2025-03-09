#include "PeerClient.h"
#include "LogWidget.h"
#include <QUuid>

PeerClient::PeerClient(QObject* parent)
	: QObject(parent), m_socket(nullptr), m_serverPort(0), m_connected(false)
{
	m_reconnectTimer = new QTimer(this);
	m_reconnectTimer->setInterval(3000);  // 每 3 秒重连一次
	m_reconnectTimer->setSingleShot(true);
	connect(m_reconnectTimer, &QTimer::timeout, this, &PeerClient::attemptReconnect);
}

PeerClient::~PeerClient()
{
	stop();
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

	// 生成 UUID 字符串并发送 RegisterPeer 消息
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

	// 解析收到的数据为 RegisterPeerResponse
	RegisterPeerResponse response;
	if (!response.ParseFromArray(data.data(), data.size())) {
		emit errorOccurred("Failed to parse response");
		return;
	}
	if (response.result() == RegisterPeerResponse::OK) {
		emit registrationResult(RegisterPeerResponse::OK);
	}
	else if (response.result() == RegisterPeerResponse::SERVER_ERROR) {
		emit registrationResult(RegisterPeerResponse::SERVER_ERROR);
	}
	else {
		qDebug() << "Unknown registration result:" << response.result();
		emit registrationResult(response.result());
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
		// 断开后启动重连
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