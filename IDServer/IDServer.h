#pragma once

#include <QtWidgets/QWidget>
#include <QStandardItemModel>
#include "ui_IDServer.h"
#include <QTimer>
#include "UserInfoDB.h"
#include "RendezvousServer.h"

class IDServer : public QWidget
{
    Q_OBJECT

public:
    IDServer(QWidget *parent = nullptr);
    ~IDServer();

private slots:
	void onStartClicked();
	void onStopClicked();
	void onRegistrationSuccess(const QString& uuid, const QString& ip);
    void onConnectionDisconnected(const QString& uuid);

private:
    Ui::IDServerClass ui;
    UserInfoDB userInfoDB;
    QStandardItemModel* model;
    RendezvousServer* server_;
};
