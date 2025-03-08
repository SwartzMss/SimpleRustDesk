#ifndef UDPHEARTBEATSERER_H
#define UDPHEARTBEATSERER_H

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include "rendezvous.pb.h"  // Protobuf 生成的头文件

class UdpHeartbeatServer : public QObject {
	Q_OBJECT
public:
	explicit UdpHeartbeatServer(QObject* parent = nullptr);
	~UdpHeartbeatServer();

	// 启动 UDP 服务器，绑定指定端口
	bool start(quint16 port);

signals:
	// 可选：当收到心跳或发送回复时发出信号（方便日志或其他处理）
	void heartbeatReceived(const QString& address, quint16 port);
	void heartbeatSent(const QString& address, quint16 port);

private slots:
	void onReadyRead();

private:
	QUdpSocket* udpSocket;
};

#endif // UDPHEARTBEATSERER_H
