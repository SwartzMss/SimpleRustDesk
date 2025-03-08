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
	// ƥ�䲢�����м�
	void tryPairing(const QString& uuid, std::shared_ptr<ConnectionHandler> handler);

private slots :
	void start();
    void stop();

private:
    Ui::RelayServerClass ui;

	QTcpServer m_server;
	// �洢��ƥ������ӣ�key Ϊ uuid ��������ʶ��
	QMap<QString, std::shared_ptr<ConnectionHandler>> mPeers;
	UdpHeartbeatServer* m_udpHeartbeatServer;
};
