#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

// 注册超时设置（示例中仅用于参考，可根据需要扩展业务逻辑）
const int REG_TIMEOUT_MS = 30000;

MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent)
{
}

// 辅助函数，构造 RegisterPeerResponse 消息并序列化为 QByteArray
QByteArray MessageProcessor::createRegisterPeerResponse(RegisterPeerResponse::Result result) {
	RegisterPeerResponse response;
	response.set_result(result);
	RendezvousMessage msg;
	// 将 RegisterPeerResponse 填充到 oneof 字段中
	msg.mutable_register_peer_response()->CopyFrom(response);
	QByteArray out;
	out.resize(msg.ByteSizeLong());
	msg.SerializeToArray(out.data(), out.size());
	return out;
}

void MessageProcessor::processMessage(const QByteArray& data, const QHostAddress& sender, quint16 senderPort) {
	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.data(), data.size())) {
		qWarning() << "Failed to parse RendezvousMessage from data";
		return;
	}
	// 根据 oneof 字段判断消息类型
	 if (msg.has_register_peer()) {
		processRegisterPeer(msg.register_peer(), sender, senderPort);
	}
	else {
		qWarning() << "Unknown message type in RendezvousMessage";
	}
}


void MessageProcessor::processRegisterPeer(const RegisterPeer& req, const QHostAddress& sender, quint16 senderPort) {
	// RegisterPeer 消息中仅包含 uuid（字节数据），这里转换为字符串处理
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());
	// 使用 uuid 作为键创建或更新 Peer
	Peer* peer = peerManager.getOrCreate(uuid);
	peer->lastRegTime = QDateTime::currentDateTime();
	peer->socketAddr = sender.toString() + ":" + QString::number(senderPort);
	// 简化示例中直接回复 OK
	QByteArray responseData = createRegisterPeerResponse(RegisterPeerResponse::OK);
	emit sendResponse(responseData, sender, senderPort);
	qDebug() << "Processed RegisterPeer, uuid:" << uuid;
}
