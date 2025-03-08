#include "MessageHandler.h"
#include <QDebug>

MessageHandler::MessageHandler(QObject* parent)
	: QObject(parent)
{
}

MessageHandler::~MessageHandler()
{
}

QByteArray MessageHandler::createPunchHoleRequestMessage(const QString& uuid)
{
	currentUuid = uuid;

	PunchHoleRequest request;
	// 设置 uuid 字段（转换为字节数据）
	request.set_uuid(uuid.toUtf8().constData(), uuid.toUtf8().size());

	RendezvousMessage msg;
	msg.mutable_punch_hole_request()->CopyFrom(request);

	QByteArray data;
	data.resize(msg.ByteSizeLong());
	msg.SerializeToArray(data.data(), data.size());
	return data;
}

void MessageHandler::processReceivedData(const QByteArray& data)
{
	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.data(), data.size())) {
		emit parseError("Failed to parse RendezvousMessage");
		return;
	}

	if (msg.has_punch_hole_response()) {
		const PunchHoleResponse& response = msg.punch_hole_response();
		QString relayServer = QString::fromUtf8(response.relay_server().data(), response.relay_server().size());
		int relayPort = response.relay_port();
		int result = static_cast<int>(response.result());  // 枚举值：OK=0, ID_NOT_EXIST=1, OFFLINE=2
		emit punchHoleResponseReceived(relayServer, relayPort, result);
	}
	else {
		emit parseError("Received unknown message type");
	}
}
