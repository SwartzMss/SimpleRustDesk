#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "LogWidget.h"
#include <QDateTime>


MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent)
{
}



void MessageProcessor::processMessage(const QByteArray& data, QTcpSocket* socket) {
	RendezvousMessage msg;
	if (!msg.ParseFromArray(data.data(), data.size())) {
		LogWidget::instance()->addLog("Failed to parse RendezvousMessage from data", LogWidget::Warning);
		return;
	}
	// 根据 oneof 字段判断消息类型
	 if (msg.has_register_peer()) {
		emit registerPeer(msg.register_peer(), socket);
	}
	 else if (msg.has_punch_hole_request()) {
		 emit punchHoleRequest(msg.punch_hole_request(), socket);
		}
	 else if (msg.has_punch_hole_sent()) {
		 emit punchHoleSent(msg.punch_hole_sent(), socket);
	 }
	 else {
		LogWidget::instance()->addLog("Unknown message type in RendezvousMessage", LogWidget::Warning);
	}
}
