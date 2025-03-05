#pragma once

#include <QWidget>
#include "ui_DeskServer.h"
#include "PeerClient.h"


class DeskServer : public QWidget
{
	Q_OBJECT

public:
	explicit DeskServer(QWidget* parent = nullptr);
	~DeskServer();

private slots:
	void onStartClicked();
	void onStopClicked();
	void onRegistrationResult(int result);
	void onClientError(const QString& errorString);

private:
	Ui::DeskServerClass ui;
	PeerClient* m_peerClient;
};
