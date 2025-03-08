#ifndef UDPHEARTBEATSERER_H
#define UDPHEARTBEATSERER_H

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include "rendezvous.pb.h"  // Protobuf ���ɵ�ͷ�ļ�

class UdpHeartbeatServer : public QObject {
	Q_OBJECT
public:
	explicit UdpHeartbeatServer(QObject* parent = nullptr);
	~UdpHeartbeatServer();

	// ���� UDP ����������ָ���˿�
	bool start(quint16 port);

signals:
	// ��ѡ�����յ��������ͻظ�ʱ�����źţ�������־����������
	void heartbeatReceived(const QString& address, quint16 port);
	void heartbeatSent(const QString& address, quint16 port);

private slots:
	void onReadyRead();

private:
	QUdpSocket* udpSocket;
};

#endif // UDPHEARTBEATSERER_H
