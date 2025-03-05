#ifndef PEER_H
#define PEER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QtNetwork/QTcpSocket>

struct Peer {
	QString id;
	QDateTime lastRegTime;
	QString socketAddr; // 格式为 "ip:port"
	QString uuid;
	QString pk;
	QJsonObject info;   // 至少保存 "ip" 字段
	int regPkCount;     // 注册 pk 的累计次数
	QDateTime regPkTime;// 上次注册 pk 的时间
	QTcpSocket* tcpSocket; // 如果有 TCP 连接，存储对应指针

	Peer();
};

#endif // PEER_H
