#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include "rendezvous.pb.h"

class MessageProcessor : public QObject {
	Q_OBJECT
public:
	explicit MessageProcessor(QObject* parent = nullptr);

	// 处理接收到的消息（data 格式为 JSON）
	void processMessage(const QByteArray& data, QTcpSocket* sockett);
signals:
	// 发出发送回复的信号，由上层网络模块调用发送函数
	void sendResponse(QTcpSocket* socket,const QByteArray& data);
private:
	// 各种消息处理分支
	void processRegisterPeer(const RegisterPeer& req, QTcpSocket* socket);

	QByteArray createRegisterPeerResponse(RegisterPeerResponse::Result result);
};

#endif // MESSAGEPROCESSOR_H
