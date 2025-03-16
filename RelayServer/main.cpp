#include "RelayServer.h"
#include <QtWidgets/QApplication>
#include <QtNetwork/QNetworkProxy>

#include <QSharedMemory>

bool checkSingleInstance(const QString& key) {
	static QSharedMemory sharedMem(key);
	if (!sharedMem.create(1)) {
		return false; // ����ʵ��������
	}
	return true; // û��ʵ��������
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
    // ��ֹ���
    w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
