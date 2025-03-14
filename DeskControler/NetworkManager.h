#pragma once

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "MessageHandler.h"

class NetworkManager : public QObject
{
	Q_OBJECT
public:
	explicit NetworkManager(QObject* parent = nullptr);
	~NetworkManager();

	// 建立 TCP 连接
	bool connectToServer(const QString& ip, quint16 port);
	// 发送 PunchHoleRequest 消息（内部调用 MessageHandler）
	void sendPunchHoleRequest(const QString& uuid);

signals:
	// 直接将内部 MessageHandler 解析的 PunchHoleResponse 结果反馈给 UI 层
	void punchHoleResponseReceived(const QString& relayServer, int relayPort, int result);
	// 网络或消息解析错误
	void networkError(const QString& error);
	// 连接断开
	void disconnected();

private slots:
	void onReadyRead();
	void onSocketDisconnected();

private:
	QTcpSocket* socket;
	MessageHandler messageHandler;  // 内部包含消息处理逻辑
};
