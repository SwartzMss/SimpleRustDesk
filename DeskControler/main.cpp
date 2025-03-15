#include "DeskControler.h"
#include <QtWidgets/QApplication>
#include <QtCore/QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    DeskControler w;
    w.show();
    return a.exec();
}
