#include "NetworkManager.h"
#include <QDebug>
#include <QUrl>
#include <QtNetwork/QHostInfo>

NetworkManager::NetworkManager(QObject* parent)
	: QObject(parent),
	socket(new QTcpSocket(this))
{
	// 连接 QTcpSocket 信号
	connect(socket, &QTcpSocket::connected, this, &NetworkManager::onSocketConnected);
	connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
	connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::onSocketDisconnected);

	// 将 MessageHandler 内部信号转发到本类信号
	connect(&messageHandler, &MessageHandler::punchHoleResponseReceived,
		this, &NetworkManager::punchHoleResponseReceived);
	connect(&messageHandler, &MessageHandler::parseError,
		this, [this](const QString& error) { emit networkError(error); });
}

NetworkManager::~NetworkManager()
{
	if (socket->state() != QAbstractSocket::UnconnectedState)
		socket->disconnectFromHost();
}

bool NetworkManager::connectToServer(const QString& ip, quint16 port)
{
	// 解析输入，支持 URL 格式，如 "http://example.com"
	QUrl url = QUrl::fromUserInput(ip);
	// 如果 URL 解析成功，则提取 host，否则使用原始字符串
	QString host = url.host().isEmpty() ? ip : url.host();

	QHostAddress resolvedAddress;
	// 尝试直接将 host 转换为 IP 地址
	if (!resolvedAddress.setAddress(host)) {
		// 转换失败，进行 DNS 解析
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
			// 解析失败，返回 false
			return false;
		}
		resolvedAddress = info.addresses().first();
	}

	// 使用解析后的地址建立连接
	socket->connectToHost(resolvedAddress, port);
	if (!socket->waitForConnected(500)) {
		return false;
	}
	return true;
}

void NetworkManager::sendPunchHoleRequest(const QString& uuid)
{
	QByteArray data = messageHandler.createPunchHoleRequestMessage(uuid);
	if (socket->state() == QAbstractSocket::ConnectedState) {
		socket->write(data);
	}
}

void NetworkManager::onSocketConnected()
{
	qDebug() << "Socket connected.";
}

void NetworkManager::onReadyRead()
{
	QByteArray data = socket->readAll();
	// 将收到的数据交给 MessageHandler 解析
	messageHandler.processReceivedData(data);
}


void NetworkManager::onSocketDisconnected()
{
	emit disconnected();
}
