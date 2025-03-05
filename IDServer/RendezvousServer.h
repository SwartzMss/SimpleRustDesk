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
	// 动态启动服务器，指定监听端口，返回是否启动成功
	bool start(quint16 port);
	// 停止服务器
	void stop();
signals:
	// 由 MessageProcessor 发出的回复数据，通过本信号发送出去
	void sendResponse(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
private slots:
	void onNewTcpConnection();
	void onTcpReadyRead(QTcpSocket* socket);
	void onTcpDisconnected(QTcpSocket* socket);

private:
	QTcpServer* tcpServer;
	// 保存 TCP 连接（用于打孔时转发 RelayResponse），key 为 "ip:port"
	QHash<QString, QTcpSocket*> tcpPunchMap;
	MessageProcessor* msgProcessor;
};

#endif // RENDEZVOUSSERVER_H
