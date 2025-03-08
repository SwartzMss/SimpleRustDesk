#pragma once

#include <QtWidgets/QWidget>
#include <QtNetWork/QTcpServer>
#include <QtCore/QMap>
#include "ui_RelayServer.h"
#include "ConnectionHandler.h"
#include "UdpHeartbeatServer.h" 

class RelayServer : public QWidget
{
    Q_OBJECT

public:
    RelayServer(QWidget *parent = nullptr);
    ~RelayServer();

private:
	void handleNewConnection();
	// 匹配并进行中继
	void tryPairing(const QString& uuid, std::shared_ptr<ConnectionHandler> handler);

private slots :
	void start();
    void stop();

private:
    Ui::RelayServerClass ui;

	QTcpServer m_server;
	// 存储待匹配的连接，key 为 uuid 或其他标识符
	QMap<QString, std::shared_ptr<ConnectionHandler>> mPeers;
	UdpHeartbeatServer* m_udpHeartbeatServer;
};
