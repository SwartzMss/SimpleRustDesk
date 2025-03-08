#include "RendezvousServer.h"
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include "LogWidget.h"

RendezvousServer::RendezvousServer(const std::shared_ptr<UserInfoDB> db, QObject* parent)
	: QObject(parent) , tcpServer(nullptr)
{
	msgProcessor = new MessageProcessor(db,this);

	connect(msgProcessor, &MessageProcessor::sendResponse,
		this, &RendezvousServer::handleSendResponse);
}

bool RendezvousServer::start(quint16 port) {

	tcpServer = new QTcpServer(this);
	// 启动 TCP 服务器
	bool tcpOk = tcpServer->listen(QHostAddress::Any, port);
	if (!tcpOk) {
		LogWidget::instance()->addLog(QString("TCP Server error: %1").arg(tcpServer->errorString()), LogWidget::Error);
		return false;
	}
	connect(tcpServer, &QTcpServer::newConnection, this, &RendezvousServer::onNewTcpConnection);

	return true;
}

void RendezvousServer::stop() {
	if (tcpServer) {
		tcpServer->close();
		tcpServer->deleteLater();
		tcpServer = nullptr;
	}
	tcpPunchMap.clear();
}


void RendezvousServer::handleSendResponse(QTcpSocket* tcpSocket, const QByteArray& data) {

	tcpSocket->write(data);

	// 解析返回的数据
	RendezvousMessage msg;
	if (msg.ParseFromArray(data.data(), data.size())) {
		if (msg.has_register_peer_response() &&
			msg.register_peer_response().result() == RegisterPeerResponse::OK) {
			// 假设在处理注册时已把 uuid 存入了 socket 的属性中
			QVariant uuidVar = tcpSocket->property("uuid");
			if (uuidVar.isValid()) {
				QString uuid = uuidVar.toString();
				QString ip = tcpSocket->peerAddress().toString();
				tcpPunchMap.insert(uuid, tcpSocket);
				// 发射信号，通知上层处理数据库和UI更新
				emit registrationSuccess(uuid, ip);
			}
		}
	}
}

void RendezvousServer::onNewTcpConnection() {
	while (tcpServer->hasPendingConnections()) {
		QTcpSocket* socket = tcpServer->nextPendingConnection();
		QString peerAddr = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());
		socket->setProperty("ip", peerAddr);
		LogWidget::instance()->addLog(QString("New TCP connection from : %1").arg(peerAddr), LogWidget::Info);
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { onTcpReadyRead(socket); });
		connect(socket, &QTcpSocket::disconnected, this, [this, socket]() { onTcpDisconnected(socket); });
	}
}

void RendezvousServer::onTcpReadyRead(QTcpSocket* socket) {
	QByteArray data = socket->readAll();
	msgProcessor->processMessage(data, socket);
}

void RendezvousServer::onTcpDisconnected(QTcpSocket* socket) {
	QString peerAddr = socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());
	socket->deleteLater();
	LogWidget::instance()->addLog(QString("TCP connection disconnected from : %1").arg(peerAddr), LogWidget::Info);
	QVariant uuidVar = socket->property("uuid");
	if (uuidVar.isValid())
	{
		QString uuid = uuidVar.toString();
		tcpPunchMap.remove(uuid);
		emit connectionDisconnected(uuid);
	}
}
