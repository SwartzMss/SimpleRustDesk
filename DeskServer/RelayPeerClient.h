#pragma once

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include "rendezvous.pb.h"

class RelayPeerClient : public QObject {
	Q_OBJECT
public:
	explicit RelayPeerClient(QObject* parent = nullptr);
	~RelayPeerClient();

	// 启动 RelayPeerClient，传入 Relay 的 IP 和 Port
	void start(const QHostAddress& relayAddress, quint16 relayPort);
	// 停止
	void stop();

signals:
	// 收到 Heartbeat 回复时发出信号
	void heartbeatResponseReceived();
	// 出错信号
	void errorOccurred(const QString& errorString);

private slots:
	// 定时发送 Heartbeat 消息
	void sendHeartbeat();
	// 处理收到的数据
	void onReadyRead();

private:
	QUdpSocket* m_udpSocket;
	QHostAddress m_relayAddress;
	quint16 m_relayPort;
	QTimer* m_heartbeatTimer;
};
