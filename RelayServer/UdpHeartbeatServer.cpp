#include "UdpHeartbeatServer.h"
#include <QtNetwork/QNetworkDatagram>
#include <QDebug>

UdpHeartbeatServer::UdpHeartbeatServer(QObject* parent)
	: QObject(parent)
{
	udpSocket = new QUdpSocket(this);
}

UdpHeartbeatServer::~UdpHeartbeatServer()
{
	udpSocket->close();
}

bool UdpHeartbeatServer::start(quint16 port)
{
	// �󶨵������ַ��ָ���˿�
	if (!udpSocket->bind(QHostAddress::Any, port)) {
		qWarning() << "UDP socket bind failed:" << udpSocket->errorString();
		return false;
	}
	connect(udpSocket, &QUdpSocket::readyRead, this, &UdpHeartbeatServer::onReadyRead);
	qDebug() << "UDP server started on port" << port;
	return true;
}

void UdpHeartbeatServer::onReadyRead()
{
	// ѭ���������й�������ݱ�
	while (udpSocket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = udpSocket->receiveDatagram();
		QByteArray data = datagram.data();

		// ����Ϊ RendezvousMessage
		RendezvousMessage msg;
		if (!msg.ParseFromArray(data.data(), data.size())) {
			qWarning() << "Failed to parse RendezvousMessage from UDP data";
			continue;
		}

		// ����� Heartbeat ��Ϣ
		if (msg.has_heartbeat()) {
			QString senderAddress = datagram.senderAddress().toString();
			quint16 senderPort = datagram.senderPort();
			qDebug() << "Received Heartbeat from" << senderAddress << ":" << senderPort;
			emit heartbeatReceived(senderAddress, senderPort);

			// ����һ�� Heartbeat �ظ���Ϣ
			RendezvousMessage replyMsg;
			replyMsg.mutable_heartbeat(); // ���� heartbeat �ֶ�

			QByteArray replyData;
			replyData.resize(replyMsg.ByteSizeLong());
			replyMsg.SerializeToArray(replyData.data(), replyData.size());

			// ���ͻظ���ԭ������
			udpSocket->writeDatagram(replyData, datagram.senderAddress(), senderPort);
			qDebug() << "Sent Heartbeat reply to" << senderAddress << ":" << senderPort;
			emit heartbeatSent(senderAddress, senderPort);
		}
	}
}
