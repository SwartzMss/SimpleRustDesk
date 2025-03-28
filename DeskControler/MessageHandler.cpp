#include "MessageHandler.h"
#include <QUuid>

MessageHandler::MessageHandler(QObject* parent)
	: QObject(parent)
{
}

MessageHandler::~MessageHandler()
{
}

QByteArray MessageHandler::createPunchHoleRequestMessage(const QString& uuid)
{
	QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);

	PunchHoleRequest request;
	request.set_uuid(uuid.toUtf8().constData(), uuid.toUtf8().size());
	request.set_id(uuidStr.toUtf8().constData(), uuidStr.toUtf8().size());

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
		int result = static_cast<int>(response.result());
		emit punchHoleResponseReceived(relayServer, relayPort, result);
	}
	else if (msg.has_inpuvideoframe()) {
		const InpuVideoFrame& frame = msg.inpuvideoframe();
		emit InpuVideoFrameReceived(QByteArray::fromStdString(frame.data()));
	}
	else if (msg.has_clipboardevent()) {
		const ClipboardEvent& clipboardEvent = msg.clipboardevent();
		emit onClipboardMessageReceived(clipboardEvent);
	}
	else {
		emit parseError("Received unknown message type");
	}
}
