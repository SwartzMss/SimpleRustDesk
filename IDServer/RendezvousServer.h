#ifndef RENDEZVOUSSERVER_H
#define RENDEZVOUSSERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QHash>
#include "MessageProcessor.h"


class RendezvousServer : public QObject {
	Q_OBJECT
public:
	explicit RendezvousServer(QObject* parent = nullptr);
	// ��̬������������ָ�������˿ڣ������Ƿ������ɹ�
	bool start(quint16 port);
	// ֹͣ������
	void stop();


signals:
	void registrationSuccess(const QString& uuid, const QString& ip);
	void connectionDisconnected(const QString& uuid);

private slots:
	void handleSendResponse(QTcpSocket* tcpSocket, const QByteArray& data);
	void onNewTcpConnection();
	void onTcpReadyRead(QTcpSocket* socket);
	void onTcpDisconnected(QTcpSocket* socket);

private:
	QTcpServer* tcpServer;
	// ���� TCP ���ӣ����ڴ��ʱת�� RelayResponse����key Ϊ "ip:port"
	QHash<QString, QTcpSocket*> tcpPunchMap;
	MessageProcessor* msgProcessor;
};

#endif // RENDEZVOUSSERVER_H
