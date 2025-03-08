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
	explicit MessageProcessor(const std::shared_ptr<UserInfoDB> db, QObject* parent = nullptr);

	// ������յ�����Ϣ��data ��ʽΪ JSON��
	void processMessage(const QByteArray& data, QTcpSocket* sockett);
signals:
	// �������ͻظ����źţ����ϲ�����ģ����÷��ͺ���
	void sendResponse(QTcpSocket* socket,const QByteArray& data);
private:
	// ������Ϣ�����֧
	void processRegisterPeer(const RegisterPeer& req, QTcpSocket* socket);

	void processPunchHoleRequest(const PunchHoleRequest& req, QTcpSocket* socket);

	QByteArray createRegisterPeerResponse(RegisterPeerResponse::Result result);

private:
	std::shared_ptr<UserInfoDB> userInfoDB;
};

#endif // MESSAGEPROCESSOR_H
