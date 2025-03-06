#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

// ע�ᳬʱ���ã�ʾ���н����ڲο����ɸ�����Ҫ��չҵ���߼���
const int REG_TIMEOUT_MS = 30000;

MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent)
{
}

// �������������� RegisterPeerResponse ��Ϣ�����л�Ϊ QByteArray
QByteArray MessageProcessor::createRegisterPeerResponse(RegisterPeerResponse::Result result) {
	RegisterPeerResponse response;
	response.set_result(result);
	RendezvousMessage msg;
	// �� RegisterPeerResponse ��䵽 oneof �ֶ���
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
	// ���� oneof �ֶ��ж���Ϣ����
	 if (msg.has_register_peer()) {
		processRegisterPeer(msg.register_peer(), sender, senderPort);
	}
	else {
		qWarning() << "Unknown message type in RendezvousMessage";
	}
}


void MessageProcessor::processRegisterPeer(const RegisterPeer& req, const QHostAddress& sender, quint16 senderPort) {
	// RegisterPeer ��Ϣ�н����� uuid���ֽ����ݣ�������ת��Ϊ�ַ�������
	QString uuid = QString::fromUtf8(req.uuid().data(), req.uuid().size());
	// ʹ�� uuid ��Ϊ����������� Peer
	Peer* peer = peerManager.getOrCreate(uuid);
	peer->lastRegTime = QDateTime::currentDateTime();
	peer->socketAddr = sender.toString() + ":" + QString::number(senderPort);
	// ��ʾ����ֱ�ӻظ� OK
	QByteArray responseData = createRegisterPeerResponse(RegisterPeerResponse::OK);
	emit sendResponse(responseData, sender, senderPort);
	qDebug() << "Processed RegisterPeer, uuid:" << uuid;
}
