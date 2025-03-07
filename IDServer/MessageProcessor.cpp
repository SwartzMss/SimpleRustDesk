#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "LogWidget.h"
#include <QDateTime>

const int REG_TIMEOUT_MS = 30000;

MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent)
{
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
	// ���� oneof �ֶ��ж���Ϣ����
	 if (msg.has_register_peer()) {
		processRegisterPeer(msg.register_peer(), socket);
	}
	else {
		LogWidget::instance()->addLog("Unknown message type in RendezvousMessage", LogWidget::Warning);
	}
}


void MessageProcessor::processRegisterPeer(const RegisterPeer& req, QTcpSocket* socket) {
	// RegisterPeer ��Ϣ�н����� uuid���ֽ����ݣ�������ת��Ϊ�ַ�������
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());
	socket->setProperty("uuid", uuid);
	QByteArray responseData = createRegisterPeerResponse(RegisterPeerResponse::OK);
	emit sendResponse(socket,responseData);

}
