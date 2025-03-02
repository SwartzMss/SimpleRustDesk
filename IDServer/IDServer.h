#pragma once

#include <QtWidgets/QWidget>
#include "ui_IDServer.h"

class IDServer : public QWidget
{
    Q_OBJECT

public:
    IDServer(QWidget *parent = nullptr);
    ~IDServer();

private:
    Ui::IDServerClass ui;
};
