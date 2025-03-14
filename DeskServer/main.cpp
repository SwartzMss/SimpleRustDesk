#include "DeskServer.h"
#include <QtWidgets/QApplication>
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

	const QString sharedMemoryKey = "DeskServerSharedMemory";

	QSharedMemory sharedMem(sharedMemoryKey);
	if (!sharedMem.create(1)) {
		return 0;
	}
    DeskServer w;
	// ��ֹ���
	w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
