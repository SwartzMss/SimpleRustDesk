#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "LogWidget.h"
#include <QDateTime>

const int REG_TIMEOUT_MS = 30000;

MessageProcessor::MessageProcessor(const std::shared_ptr<UserInfoDB> db,QObject* parent) : QObject(parent)
{
	userInfoDB = db;
}

QByteArray MessageProcessor::createRegisterPeerResponse(RegisterPeerResponse::Result result) {
	RegisterPeerResponse response;
	response.set_result(result);
	RendezvousMessage msg;
	msg.mutable_register_peer_response()->CopyFrom(response);
	QByteArray out;
	out.resize(msg.ByteSizeLong());
	msg.SerializeToArray(out.data(), out.size());
	return out;
}

void MessageProcessor::processMessage(const QByteArray& data, QTcpSocket* socket) {
	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.data(), data.size())) {
		LogWidget::instance()->addLog("Failed to parse RendezvousMessage from data", LogWidget::Warning);
		return;
	}
	// 根据 oneof 字段判断消息类型
	 if (msg.has_register_peer()) {
		processRegisterPeer(msg.register_peer(), socket);
	}
	 else if (msg.has_punch_hole_request()) {
		 processPunchHoleRequest(msg.punch_hole_request(), socket);
		}
	 else {
		LogWidget::instance()->addLog("Unknown message type in RendezvousMessage", LogWidget::Warning);
	}
}


void MessageProcessor::processPunchHoleRequest(const PunchHoleRequest& req, QTcpSocket* socket) 
{
	// 将 uuid 从字节数据转换为字符串
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());

	bool idExists = false;
	auto allUsers = userInfoDB->getAllUserInfo();
	for (const auto& user : allUsers) {
		if (QString::fromStdString(user.UUID) == uuid) {
			idExists = true;
			break;
		}
	}

	PunchHoleResponse response;
	if (idExists) {
		response.set_relay_server("relay.example.com");
		response.set_relay_port(12345);
		response.set_result(PunchHoleResponse::OK);
	}
	else {
		// 如果 ID 不存在，返回相应的错误结果
		response.set_result(PunchHoleResponse::ID_NOT_EXIST);
	}

	RendezvousMessage msg;
	msg.mutable_punch_hole_response()->CopyFrom(response);

	QByteArray out;
	out.resize(msg.ByteSizeLong());
	msg.SerializeToArray(out.data(), out.size());

	emit sendResponse(socket, out);
}

void MessageProcessor::processRegisterPeer(const RegisterPeer& req, QTcpSocket* socket) {
	// RegisterPeer 消息中仅包含 uuid（字节数据），这里转换为字符串处理
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());
	socket->setProperty("uuid", uuid);
	QByteArray responseData = createRegisterPeerResponse(RegisterPeerResponse::OK);
	emit sendResponse(socket,responseData);

}
