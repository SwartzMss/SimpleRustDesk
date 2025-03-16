#include "DeskControler.h"
#include <QtWidgets/QApplication>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    DeskControler w;
    w.show();
    return a.exec();
}
