#include "IDServer.h"
#include <QtWidgets/QApplication>
#include <QSharedMemory>
#include <QtCore/QDir>
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
	QDir::setCurrent(a.applicationDirPath());
	const QString sharedMemoryKey = "IDServerSharedMemory";

	QSharedMemory sharedMem(sharedMemoryKey);
	if (!sharedMem.create(1)) {
		return 0;
	}
	IDServer w;

	// ��ֹ���
	w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
