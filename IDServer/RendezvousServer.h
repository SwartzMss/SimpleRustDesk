#ifndef RENDEZVOUSSERVER_H
#define RENDEZVOUSSERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QHash>
#include "UserInfoDB.h"
#include "MessageProcessor.h"


class RendezvousServer : public QObject {
	Q_OBJECT
public:
	explicit RendezvousServer(const std::shared_ptr<UserInfoDB> db,QObject* parent = nullptr);
	// ��̬������������ָ�������˿ڣ������Ƿ������ɹ�
	bool start(quint16 port);
	// ֹͣ������
	void stop();


signals:
	void registrationSuccess(const QString& uuid, const QString& ip);
	void connectionDisconnected(const QString& uuid);

private slots:
	void onNewTcpConnection();
	void onTcpReadyRead(QTcpSocket* socket);
	void onTcpDisconnected(QTcpSocket* socket);
	void handlePunchHoleRequest(const PunchHoleRequest& req, QTcpSocket* socket);
	void handleRegisterPeer(const RegisterPeer& req, QTcpSocket* socket);
	void handlePunchHoleSent(const PunchHoleSent& req, QTcpSocket* socket);

private:
	QTcpServer* tcpServer;
	QHash<QString, QTcpSocket*> tcpPunchMap;
	MessageProcessor* msgProcessor;
	std::shared_ptr<UserInfoDB> userInfoDB;

};

#endif // RENDEZVOUSSERVER_H
