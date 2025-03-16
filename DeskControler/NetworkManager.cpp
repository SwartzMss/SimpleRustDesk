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
}

NetworkManager::~NetworkManager()
{
	cleanup();
}

void NetworkManager::cleanup()
{
	if (socket) {
		socket->disconnect();
		if (socket->state() != QAbstractSocket::UnconnectedState) {
			socket->disconnectFromHost();
		}
		socket->deleteLater();
		socket = nullptr;
	}
}


bool NetworkManager::connectToServer(const QString& ip, quint16 port)
{
	// 解析输入，支持 URL 格式
	QUrl url = QUrl::fromUserInput(ip);
	// 如果 URL 解析成功，则提取 host，否则使用原始字符串
	QString host = url.host().isEmpty() ? ip : url.host();

	QHostAddress resolvedAddress;
	if (!resolvedAddress.setAddress(host)) {
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
			QString error = QString("Failed to resolve host: %1").arg(host);
			LogWidget::instance()->addLog(error, LogWidget::Error);
			return false;
		}
		// 遍历地址列表，筛选 IPv4 地址
		bool foundIPv4 = false;
		for (const QHostAddress& address : info.addresses()) {
			if (address.protocol() == QAbstractSocket::IPv4Protocol) {
				resolvedAddress = address;
				foundIPv4 = true;
				break;
			}
		}
		if (!foundIPv4) {
			LogWidget::instance()->addLog("No IPv4 address found for Relay IP: " + host, LogWidget::Error);
			return false;
		}
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

	quint32 packetSize = static_cast<quint32>(data.size());
	quint32 bigEndianSize = qToBigEndian(packetSize);
	QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

	QByteArray fullData;
	fullData.append(header);
	fullData.append(data);

	if (socket->state() == QAbstractSocket::ConnectedState) {
		socket->write(fullData);
		LogWidget::instance()->addLog(QString("Punch hole request sent for UUID: %1").arg(uuid), LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog("Failed to send Punch Hole Request: socket not connected", LogWidget::Warning);
	}
}

void NetworkManager::onReadyRead()
{
	m_buffer.append(socket->readAll());

	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		if (m_buffer.size() < 4 + static_cast<int>(packetSize))
			break;

		QByteArray packetData = m_buffer.mid(4, packetSize);

		m_buffer.remove(0, 4 + packetSize);

		messageHandler.processReceivedData(packetData);
	}
}


void NetworkManager::onSocketDisconnected()
{
	socket->deleteLater();
	socket = nullptr;
	emit disconnected();
}
