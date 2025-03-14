#include "NetworkWorker.h"
#include "LogWidget.h"
#include "rendezvous.pb.h"

#include <QtEndian>

NetworkWorker::NetworkWorker(QObject* parent)
	: QObject(parent)
{
}

NetworkWorker::~NetworkWorker()
{
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
	}
}

void NetworkWorker::connectToServer(const QString& host, quint16 port, const QString& uuid)
{
	m_host = host;
	m_port = port;
	m_uuid = uuid;

	if (m_socket) {
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	m_socket = new QTcpSocket(this);

	// ���� socket �źŵ������
	connect(m_socket, &QTcpSocket::connected, this, &NetworkWorker::onSocketConnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &NetworkWorker::onSocketReadyRead);
	connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkWorker::onSocketError);
	connect(m_socket, &QTcpSocket::disconnected, this, &NetworkWorker::onSocketDisconnected);

	// ��������
	m_socket->connectToHost(host, port);
}

void NetworkWorker::onSocketConnected()
{
	QString info = QString("Disconnected from server [%1:%2]").arg(m_host).arg(m_socket->peerPort());
	LogWidget::instance()->addLog(info, LogWidget::Warning);
	emit networkError(info);
}

void NetworkWorker::sendRequestRelay()
{
	RendezvousMessage msg;
	RequestRelay* req = msg.mutable_request_relay();
	req->set_uuid(m_uuid.toUtf8().constData(), m_uuid.toUtf8().size());

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		LogWidget::instance()->addLog("Failed to serialize RequestRelay message", LogWidget::Error);
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(data);
		m_socket->flush();
		LogWidget::instance()->addLog(QString("Sent RequestRelay to [%1:%2] with uuid=%3")
			.arg(m_socket->peerAddress().toString())
			.arg(m_socket->peerPort())
			.arg(m_uuid), LogWidget::Info);
	}
}

void NetworkWorker::onSocketReadyRead()
{
	// ���յ�������׷�ӵ�������
	m_buffer.append(m_socket->readAll());

	// Э�飺 [4�ֽڴ�������] + [������]
	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		// ������ݲ���һ�������� break
		if (m_buffer.size() < 4 + (int)packetSize) {
			break;
		}

		// ��һ֡����
		QByteArray packetData = m_buffer.mid(4, packetSize);
		m_buffer.remove(0, 4 + packetSize);

		// �����źŸ������߳�
		emit packetReady(packetData);
	}
}


void NetworkWorker::onSocketError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	if (m_socket) {
		QString err = QString("Socket Error [%1:%2]: %3")
			.arg(m_host).arg(m_socket->peerPort()).arg(m_socket->errorString());
		LogWidget::instance()->addLog(err, LogWidget::Error);
		emit networkError(err);
	}
}

void NetworkWorker::onSocketDisconnected()
{
	QString info = QString("Socket disconnected from [%1:%2]").arg(m_socket->peerAddress().toString()).arg(m_socket->peerPort());
	LogWidget::instance()->addLog(info, LogWidget::Warning);
	emit networkError(info);
}
