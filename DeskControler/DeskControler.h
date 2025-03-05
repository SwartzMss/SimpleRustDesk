#pragma once

#include <QtWidgets/QWidget>
#include "ui_DeskControler.h"

class DeskControler : public QWidget
{
    Q_OBJECT

public:
    DeskControler(QWidget *parent = nullptr);
    ~DeskControler();

private:
    Ui::DeskControlerClass ui;
};
