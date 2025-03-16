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
	QString id = QString::fromUtf8(req.id().data(), req.id().size());

	tcpPunchMap.insert(id, socket);

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
			response.set_result(Result::ID_NOT_EXIST);
		}
		else
		{
			response.set_result(Result::DESKSERVER_OFFLINE);
		}

		RendezvousMessage msg;
		msg.mutable_punch_hole_response()->CopyFrom(response);

		QByteArray out;
		out.resize(msg.ByteSizeLong());
		msg.SerializeToArray(out.data(), out.size());

		// 添加4字节长度头
		quint32 packetSize = static_cast<quint32>(out.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

		QByteArray fullData;
		fullData.append(header);
		fullData.append(out);

		socket->write(fullData);
	}
	else {
		PunchHole punchHole;
		punchHole.set_id(id.toUtf8().constData(), id.toUtf8().size());
		RendezvousMessage msg;
		msg.mutable_punch_hole()->CopyFrom(punchHole);

		QByteArray out;
		out.resize(msg.ByteSizeLong());
		msg.SerializeToArray(out.data(), out.size());

		// 添加4字节长度头
		quint32 packetSize = static_cast<quint32>(out.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

		QByteArray fullData;
		fullData.append(header);
		fullData.append(out);

		QTcpSocket* targetSocket = tcpPunchMap.value(uuid);
		targetSocket->write(fullData);
	}
}


void RendezvousServer::handleRegisterPeer(const RegisterPeer& req, QTcpSocket* socket)
{
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());

	RegisterPeerResponse response;
	response.set_result(Result::OK);
	RendezvousMessage msg;
	msg.mutable_register_peer_response()->CopyFrom(response);

	QByteArray out;
	out.resize(msg.ByteSizeLong());
	msg.SerializeToArray(out.data(), out.size());

	quint32 packetSize = static_cast<quint32>(out.size());
	quint32 bigEndianSize = qToBigEndian(packetSize);
	QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

	QByteArray fullData;
	fullData.append(header);
	fullData.append(out);

	QString ip = socket->peerAddress().toString();
	tcpPunchMap.insert(uuid, socket);
	socket->write(fullData);
	socket->setProperty("uuid", uuid);
	// 发射信号，通知上层处理数据库和UI更新
	emit registrationSuccess(uuid, ip);
}


void RendezvousServer::handlePunchHoleSent(const PunchHoleSent& req, QTcpSocket* socket)
{
	QString id = QString::fromStdString(req.id());
	auto it = tcpPunchMap.find(id);
	if (it != tcpPunchMap.end()) {
		QTcpSocket* targetSocket = it.value();

		PunchHoleResponse response;
		response.set_relay_port(req.relay_port());
		response.set_result(req.result());
		response.set_relay_server(req.relay_server());

		RendezvousMessage msg;
		msg.mutable_punch_hole_response()->CopyFrom(response);

		QByteArray out;
		out.resize(msg.ByteSizeLong());
		msg.SerializeToArray(out.data(), out.size());

		// 构造4字节大端序长度头
		quint32 packetSize = static_cast<quint32>(out.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

		// 组合完整数据：[长度头][消息数据]
		QByteArray fullData;
		fullData.append(header);
		fullData.append(out);

		targetSocket->write(fullData);
	}
	else {
		LogWidget::instance()->addLog("PunchHoleSent would be dropped", LogWidget::Warning);
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

void RendezvousServer::onTcpReadyRead(QTcpSocket* socket)
{
	QByteArray buffer = socket->property("buffer").toByteArray();

	buffer.append(socket->readAll());

	while (buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		if (buffer.size() < 4 + static_cast<int>(packetSize))
			break;

		QByteArray messageData = buffer.mid(4, packetSize);

		buffer.remove(0, 4 + packetSize);

		msgProcessor->processMessage(messageData, socket);
	}

	socket->setProperty("buffer", buffer);
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
