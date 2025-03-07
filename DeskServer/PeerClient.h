#pragma once

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include "rendezvous.pb.h"

class PeerClient : public QObject {
	Q_OBJECT
public:
	explicit PeerClient(QObject* parent = nullptr);
	~PeerClient();

	// 启动连接，传入服务器 IP 和端口
	void start(const QHostAddress& address, quint16 port);
	// 停止连接
	void stop();

signals:
	// 注册结果信号，返回 RegisterPeerResponse::Result 枚举值
	void registrationResult(RegisterPeerResponse::Result result);
	// 出错信号
	void errorOccurred(const QString& errorString);

private slots:
	void onConnected();
	void onReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);
	void onDisconnected();
	void attemptReconnect();

private:
	void doConnect();

private:
	QTcpSocket* m_socket;
	QHostAddress m_serverAddress;
	quint16 m_serverPort;
	QTimer* m_reconnectTimer;
	bool m_isStopping;  // 标记是否为主动停止

};

