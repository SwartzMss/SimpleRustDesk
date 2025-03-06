#include "PeerClient.h"
#include "LogWidget.h"
#include <QUuid>

PeerClient::PeerClient(QObject* parent)
	: QObject(parent), m_socket(nullptr), m_serverPort(0)
{
}

PeerClient::~PeerClient()
{
	stop();
}

bool PeerClient::start(const QHostAddress& address, quint16 port)
{
	m_serverAddress = address;
	m_serverPort = port;

	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::connected, this, &PeerClient::onConnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &PeerClient::onReadyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &PeerClient::onDisconnected);
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
		this, SLOT(onSocketError(QAbstractSocket::SocketError)));

	m_socket->connectToHost(m_serverAddress, m_serverPort);
	if (!m_socket->waitForConnected(3000)) {
		LogWidget::instance()->addLog(QString("Connection failed: %1").arg(m_socket->errorString()), LogWidget::Warning);
		return false;
	}
	else 
	{
		LogWidget::instance()->addLog(" Connected successfully", LogWidget::Info);
		return true;
	}
}

void PeerClient::stop()
{
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
}

void PeerClient::onConnected()
{
	// 生成 UUID 字符串
	QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);

	// 构造 RegisterPeer 消息
	RegisterPeer regPeer;
	regPeer.set_uuid(uuidStr.toStdString());

	// 构造外层的 RendezvousMessage 并设置 oneof 字段 register_peer
	RendezvousMessage msg;
	// 使用 mutable_register_peer() 获取指针并赋值
	*msg.mutable_register_peer() = regPeer;

	// 序列化消息
	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		emit errorOccurred("Serialization failed");
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));

	// 发送消息
	m_socket->write(data);
	m_socket->flush();
	LogWidget::instance()->addLog(QString(" Sent RegisterPeer message with uuid %1").arg(uuidStr), LogWidget::Info);
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
	emit errorOccurred(m_socket->errorString());
}


void PeerClient::onDisconnected()
{
	emit errorOccurred("Disconnected from server");
}