#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include "rendezvous.pb.h"
#include "UserInfoDB.h"

class MessageProcessor : public QObject {
	Q_OBJECT
public:
	explicit MessageProcessor( QObject* parent = nullptr);

	// 处理接收到的消息（data 格式为 JSON）
	void processMessage(const QByteArray& data, QTcpSocket* sockett);
signals:
	void registerPeer(const RegisterPeer& req, QTcpSocket* socket);
	void punchHoleRequest(const PunchHoleRequest& req, QTcpSocket* socket);
	void punchHoleSent(const PunchHoleSent& req, QTcpSocket* socket);
};

#endif // MESSAGEPROCESSOR_H
