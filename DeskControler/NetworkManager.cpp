#include "NetworkManager.h"
#include <QUrl>
#include <QtNetwork/QHostInfo>
#include "LogWidget.h"

NetworkManager::NetworkManager(QObject* parent)
	: QObject(parent),
	socket(new QTcpSocket(this))
{
	// ���� QTcpSocket �ź�
	connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
	connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::onSocketDisconnected);

	// �� MessageHandler �ڲ��ź�ת���������ź�
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
	// �������룬֧�� URL ��ʽ
	QUrl url = QUrl::fromUserInput(ip);
	// ��� URL �����ɹ�������ȡ host������ʹ��ԭʼ�ַ���
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
	// ʹ�ý�����ĵ�ַ��������
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
	// ���յ������ݽ��� MessageHandler ����
	messageHandler.processReceivedData(data);
}


void NetworkManager::onSocketDisconnected()
{
	emit disconnected();
}
