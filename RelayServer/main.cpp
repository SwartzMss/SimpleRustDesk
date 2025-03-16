#include "RelayServer.h"
#include <QtWidgets/QApplication>
#include <QtNetwork/QNetworkProxy>

#include <QSharedMemory>

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
	QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

	const QString sharedMemoryKey = "RelayServerSharedMemory";

	QSharedMemory sharedMem(sharedMemoryKey);
	if (!sharedMem.create(1)) {
		return 0;
	}
	RelayServer w;
    // 禁止最大化
    w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
