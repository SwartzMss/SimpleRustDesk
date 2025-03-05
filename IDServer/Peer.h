#ifndef PEER_H
#define PEER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QtNetwork/QTcpSocket>

struct Peer {
	QString id;
	QDateTime lastRegTime;
	QString socketAddr; // ��ʽΪ "ip:port"
	QString uuid;
	QString pk;
	QJsonObject info;   // ���ٱ��� "ip" �ֶ�
	int regPkCount;     // ע�� pk ���ۼƴ���
	QDateTime regPkTime;// �ϴ�ע�� pk ��ʱ��
	QTcpSocket* tcpSocket; // ����� TCP ���ӣ��洢��Ӧָ��

	Peer();
};

#endif // PEER_H
