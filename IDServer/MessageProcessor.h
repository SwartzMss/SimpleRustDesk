#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include "PeerManager.h"
#include "rendezvous.pb.h"

class MessageProcessor : public QObject {
	Q_OBJECT
public:
	explicit MessageProcessor(QObject* parent = nullptr);

	// 处理接收到的消息（data 格式为 JSON）
	void processMessage(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
signals:
	// 发出发送回复的信号，由上层网络模块调用发送函数
	void sendResponse(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
private:
	// 各种消息处理分支
	void processRegisterPeer(const RegisterPeer& req, const QHostAddress& sender, quint16 senderPort);

	QByteArray createRegisterPeerResponse(RegisterPeerResponse::Result result);
	PeerManager peerManager;
};

#endif // MESSAGEPROCESSOR_H
