#include "RendezvousServer.h"
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include "LogWidget.h"

RendezvousServer::RendezvousServer(const std::shared_ptr<UserInfoDB> db, QObject* parent)
	: QObject(parent) , tcpServer(nullptr)
{
	msgProcessor = new MessageProcessor(this);

	connect(msgProcessor, &MessageProcessor::registerPeer,
		this, &RendezvousServer::handleRegisterPeer);

	connect(msgProcessor, &MessageProcessor::punchHoleRequest,
		this, &RendezvousServer::handlePunchHoleRequest);

	connect(msgProcessor, &MessageProcessor::punchHoleSent,
		this, &RendezvousServer::handlePunchHoleSent);
	

	userInfoDB = db;	   
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

void RendezvousServer::handlePunchHoleRequest(const PunchHoleRequest& req, QTcpSocket* socket)
{
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());

	bool idExists = false;
	auto allUsers = userInfoDB->getAllUserInfo();
	for (const auto& user : allUsers) {
		if (QString::fromStdString(user.UUID) == uuid) {
			idExists = true;
			break;
		}
	}

	bool isOnline = tcpPunchMap.find(uuid) != tcpPunchMap.end();

	if (!idExists || !isOnline) {

		PunchHoleResponse response;
		if (!idExists)
		{
			response.set_result(PunchHoleResponse::ID_NOT_EXIST);
		}
		else
		{
			response.set_result(PunchHoleResponse::OFFLINE);
		}

		RendezvousMessage msg;
		msg.mutable_punch_hole_response()->CopyFrom(response);

		QByteArray out;
		out.resize(msg.ByteSizeLong());
		msg.SerializeToArray(out.data(), out.size());
		socket->write(out);
	}
	else{

		PunchHole punchHole;
		RendezvousMessage msg;
		msg.mutable_punch_hole()->CopyFrom(punchHole);

		QByteArray out;
		out.resize(msg.ByteSizeLong());
		msg.SerializeToArray(out.data(), out.size());
		QTcpSocket* socket = tcpPunchMap.value(uuid);
		socket->write(out);
	}

}

void RendezvousServer::handleRegisterPeer(const RegisterPeer& req, QTcpSocket* socket)
{
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());

	RegisterPeerResponse response;
	response.set_result(RegisterPeerResponse::OK);
	RendezvousMessage msg;
	msg.mutable_register_peer_response()->CopyFrom(response);
	QByteArray out;
	out.resize(msg.ByteSizeLong());
	msg.SerializeToArray(out.data(), out.size());

	QString ip = socket->peerAddress().toString();
	tcpPunchMap.insert(uuid, socket);
	socket->write(out);
	// 发射信号，通知上层处理数据库和UI更新
	emit registrationSuccess(uuid, ip);
}

void RendezvousServer::handlePunchHoleSent(const PunchHoleSent& req, QTcpSocket* socket)
{
	LogWidget::instance()->addLog("handlePunchHoleSent", LogWidget::Info);
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
