#include "IDServer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    IDServer w;
	// ½ûÖ¹×î´ó»¯
	w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
