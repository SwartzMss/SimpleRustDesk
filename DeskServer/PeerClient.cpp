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
	// ���� UUID �ַ���
	QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);

	// ���� RegisterPeer ��Ϣ
	RegisterPeer regPeer;
	regPeer.set_uuid(uuidStr.toStdString());

	// �������� RendezvousMessage ������ oneof �ֶ� register_peer
	RendezvousMessage msg;
	// ʹ�� mutable_register_peer() ��ȡָ�벢��ֵ
	*msg.mutable_register_peer() = regPeer;

	// ���л���Ϣ
	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		emit errorOccurred("Serialization failed");
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));

	// ������Ϣ
	m_socket->write(data);
	m_socket->flush();
	LogWidget::instance()->addLog(QString(" Sent RegisterPeer message with uuid %1").arg(uuidStr), LogWidget::Info);
}

void PeerClient::onReadyRead()
{
	QByteArray data = m_socket->readAll();

	// �����յ�������Ϊ RegisterPeerResponse
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