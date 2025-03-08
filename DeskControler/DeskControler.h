#pragma once

#include <QtWidgets/QWidget>
#include "ui_DeskControler.h"

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
	Ui::DeskControlerClass ui;
	NetworkManager* networkManager;
};
