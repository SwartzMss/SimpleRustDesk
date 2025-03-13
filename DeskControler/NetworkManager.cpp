#include "NetworkManager.h"
#include <QDebug>
#include <QUrl>
#include <QtNetwork/QHostInfo>

NetworkManager::NetworkManager(QObject* parent)
	: QObject(parent),
	socket(new QTcpSocket(this))
{
	// ���� QTcpSocket �ź�
	connect(socket, &QTcpSocket::connected, this, &NetworkManager::onSocketConnected);
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
	// �������룬֧�� URL ��ʽ���� "http://example.com"
	QUrl url = QUrl::fromUserInput(ip);
	// ��� URL �����ɹ�������ȡ host������ʹ��ԭʼ�ַ���
	QString host = url.host().isEmpty() ? ip : url.host();

	QHostAddress resolvedAddress;
	// ����ֱ�ӽ� host ת��Ϊ IP ��ַ
	if (!resolvedAddress.setAddress(host)) {
		// ת��ʧ�ܣ����� DNS ����
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
			// ����ʧ�ܣ����� false
			return false;
		}
		resolvedAddress = info.addresses().first();
	}

	// ʹ�ý�����ĵ�ַ��������
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
	// ���յ������ݽ��� MessageHandler ����
	messageHandler.processReceivedData(data);
}


void NetworkManager::onSocketDisconnected()
{
	emit disconnected();
}
