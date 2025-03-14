#include "NetworkManager.h"
#include <QUrl>
#include <QtNetwork/QHostInfo>
#include "LogWidget.h"

NetworkManager::NetworkManager(QObject* parent)
	: QObject(parent),
	socket(new QTcpSocket(this))
{
	// 连接 QTcpSocket 信号
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
	// 解析输入，支持 URL 格式
	QUrl url = QUrl::fromUserInput(ip);
	// 如果 URL 解析成功，则提取 host，否则使用原始字符串
	QString host = url.host().isEmpty() ? ip : url.host();

	LogWidget::instance()->addLog(QString("Connecting to host: %1, port: %2").arg(host).arg(port), LogWidget::Info);

	QHostAddress resolvedAddress;
	if (!resolvedAddress.setAddress(host)) {
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
			QString error = QString("Failed to resolve host: %1").arg(host);
			LogWidget::instance()->addLog(error, LogWidget::Error);
			return false;
		}
		resolvedAddress = info.addresses().first();
		LogWidget::instance()->addLog(QString("Resolved host %1 to %2").arg(host, resolvedAddress.toString()), LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog(QString("Host address directly parsed: %1").arg(host), LogWidget::Info);
	}
	// 使用解析后的地址建立连接
	socket->connectToHost(resolvedAddress, port);
	if (!socket->waitForConnected(500)) {
		QString error = QString("Connection to %1:%2 failed: %3")
			.arg(resolvedAddress.toString())
			.arg(port)
			.arg(socket->errorString());
		LogWidget::instance()->addLog(error, LogWidget::Warning);
		return false;
	}
	LogWidget::instance()->addLog(QString("Successfully connected to %1:%2").arg(resolvedAddress.toString()).arg(port), LogWidget::Info);
	return true;
}

void NetworkManager::sendPunchHoleRequest(const QString& uuid)
{
	QByteArray data = messageHandler.createPunchHoleRequestMessage(uuid);
	if (socket->state() == QAbstractSocket::ConnectedState) {
		socket->write(data);
		LogWidget::instance()->addLog(QString("Punch hole request sent for UUID: %1").arg(uuid), LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog("Failed to send Punch Hole Request: socket not connected", LogWidget::Warning);
	}
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
