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
	// 绑定到任意地址及指定端口
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
	// 循环处理所有挂起的数据报
	while (udpSocket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = udpSocket->receiveDatagram();
		QByteArray data = datagram.data();

		// 解析为 RendezvousMessage
		RendezvousMessage msg;
		if (!msg.ParseFromArray(data.data(), data.size())) {
			qWarning() << "Failed to parse RendezvousMessage from UDP data";
			continue;
		}

		// 如果是 Heartbeat 消息
		if (msg.has_heartbeat()) {
			QString senderAddress = datagram.senderAddress().toString();
			quint16 senderPort = datagram.senderPort();
			qDebug() << "Received Heartbeat from" << senderAddress << ":" << senderPort;
			emit heartbeatReceived(senderAddress, senderPort);

			// 构造一个 Heartbeat 回复消息
			RendezvousMessage replyMsg;
			replyMsg.mutable_heartbeat(); // 创建 heartbeat 字段

			QByteArray replyData;
			replyData.resize(replyMsg.ByteSizeLong());
			replyMsg.SerializeToArray(replyData.data(), replyData.size());

			// 发送回复到原发送者
			udpSocket->writeDatagram(replyData, datagram.senderAddress(), senderPort);
			qDebug() << "Sent Heartbeat reply to" << senderAddress << ":" << senderPort;
			emit heartbeatSent(senderAddress, senderPort);
		}
	}
}
