#include "RelaySocketWorker.h"

RelaySocketWorker::RelaySocketWorker(QObject* parent)
	: QObject(parent)
{
	m_socket = new QTcpSocket(this);
	m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	connect(m_socket, &QTcpSocket::connected, this, &RelaySocketWorker::socketConnected);
	connect(m_socket, &QTcpSocket::disconnected, this, &RelaySocketWorker::socketDisconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &RelaySocketWorker::onReadyRead);
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
		this, SLOT(onSocketError(QAbstractSocket::SocketError)));
}

RelaySocketWorker::~RelaySocketWorker()
{
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
	}
}

void RelaySocketWorker::connectToHost(const QHostAddress& address, quint16 port)
{
	m_socket->connectToHost(address, port);
}

void RelaySocketWorker::sendData(const QByteArray& data)
{
	if (m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(data);
		m_socket->flush();
	}
}

void RelaySocketWorker::disconnectSocket()
{
	if (m_socket) {
		m_socket->disconnectFromHost();
	}
}

void RelaySocketWorker::onReadyRead()
{
	QByteArray data = m_socket->readAll();
	emit dataReceived(data);
}

void RelaySocketWorker::onSocketError(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error);
	emit socketErrorOccurred(m_socket->errorString());
}
