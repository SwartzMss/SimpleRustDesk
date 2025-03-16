#include "IDServer.h"
#include <QtWidgets/QApplication>
#include <QSharedMemory>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QDir>
bool checkSingleInstance(const QString& key) {
	static QSharedMemory sharedMem(key);
	if (!sharedMem.create(1)) {
		return false; // 已有实例在运行
	}
	return true; // 没有实例在运行
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QDir::setCurrent(a.applicationDirPath());
	QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

	const QString sharedMemoryKey = "IDServerSharedMemory";

	QSharedMemory sharedMem(sharedMemoryKey);
	if (!sharedMem.create(1)) {
		return 0;
	}
	IDServer w;

	// 禁止最大化
	w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
