#include "IDServer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    IDServer w;
    w.show();
    return a.exec();
}
