#pragma once

#include <QWidget>
#include "ui_DeskServer.h"
#include "PeerClient.h"
#include "RelayPeerClient.h"  


class DeskServer : public QWidget
{
	Q_OBJECT

public:
	explicit DeskServer(QWidget* parent = nullptr);
	~DeskServer();

private slots:
	void onStartClicked();
	void onRegistrationResult(int result);
	void onClientError(const QString& errorString);

private:
	void updateStatus(bool online);
	void loadConfig();
	void saveConfig();


private:
	Ui::DeskServerClass ui;
	PeerClient* m_peerClient;
	RelayPeerClient* m_relayPeerClient;
};
