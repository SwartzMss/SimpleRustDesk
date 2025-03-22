#pragma once

#include <QtWidgets/QWidget>
#include "NetworkManager.h"
#include "VideoReceiver.h"
#include "ui_DeskControler.h"
#include "RemoteClipboard.h"

class NetworkManager;

class DeskControler : public QWidget
{
	Q_OBJECT

public:
	explicit DeskControler(QWidget* parent = nullptr);
	~DeskControler();

private slots:
	void onConnectClicked();
	void onPunchHoleResponse(const QString& relayServer, int relayPort, int result);
	void onNetworkError(const QString& error);
	void onNetworkDisconnected();

private:
	void setupVideoSession(const QString& relayServer, quint16 relayPort, const QString& status);
	void destroyVideoSession();

	void loadConfig();
	void saveConfig();
private:
	Ui::DeskControlerClass ui;
	NetworkManager* m_networkManager;
	VideoReceiver* m_videoReceiver;
	RemoteClipboard* m_remoteClipboard = nullptr;
};
