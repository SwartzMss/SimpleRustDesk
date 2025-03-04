#pragma once

#include <QtWidgets/QWidget>
#include <QStandardItemModel>
#include "ui_IDServer.h"
#include <QTimer>
#include "UserInfoDB.h"

class IDServer : public QWidget
{
    Q_OBJECT

public:
    IDServer(QWidget *parent = nullptr);
    ~IDServer();

private slots:
	void onStartClicked();
	void onStopClicked();
    void updateOnlineStatus();

private:
    Ui::IDServerClass ui;
    UserInfoDB userInfoDB;
    QStandardItemModel* model;
    QTimer* timer;
};
