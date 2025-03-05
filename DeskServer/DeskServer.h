#pragma once

#include <QtWidgets/QWidget>
#include "ui_DeskServer.h"

class DeskServer : public QWidget
{
    Q_OBJECT

public:
    DeskServer(QWidget *parent = nullptr);
    ~DeskServer();

private:
    Ui::DeskServerClass ui;
};
