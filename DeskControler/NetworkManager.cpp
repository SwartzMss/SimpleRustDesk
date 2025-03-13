#include "NetworkManager.h"
#include <QDebug>

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
	socket->connectToHost(ip, port);
	if (!socket->waitForConnected(500)) 
	{
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
