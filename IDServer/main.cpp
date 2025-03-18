#include "IDServer.h"
#include <QtWidgets/QApplication>
#include <QSharedMemory>
#include <QtNetwork/QNetworkProxy>
#include <QtCore/QDir>

#include <Windows.h>
#include <DbgHelp.h>
#include <tchar.h>
#include <stdio.h>

#pragma comment(lib, "Dbghelp.lib")

LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPointers)
{
	// ���� dump �ļ���������ʹ�ö�̬�������⸲��֮ǰ�� dump �ļ�
	TCHAR dumpFileName[MAX_PATH] = _T("crash_dump.dmp");

	// ���� dump �ļ�
	HANDLE hFile = CreateFile(dumpFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		// ���� dump ����
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = pExceptionPointers;
		dumpInfo.ClientPointers = FALSE;

		// ���� dump �ļ���MiniDumpNormal �������������Ϣ
		BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
			MiniDumpNormal, &dumpInfo, nullptr, nullptr);
		if (success) {
			_tprintf(_T("Dump file created: %s\n"), dumpFileName);
		}
		else {
			_tprintf(_T("MiniDumpWriteDump failed. Error: %d\n"), GetLastError());
		}
		CloseHandle(hFile);
	}
	else {
		_tprintf(_T("Failed to create dump file. Error: %d\n"), GetLastError());
	}

	// ���� EXCEPTION_EXECUTE_HANDLER��ϵͳ����ֹ����
	return EXCEPTION_EXECUTE_HANDLER;
}


bool checkSingleInstance(const QString& key) {
	static QSharedMemory sharedMem(key);
	if (!sharedMem.create(1)) {
		return false; // ����ʵ��������
	}
	return true; // û��ʵ��������
}
int main(int argc, char *argv[])
{
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
    QApplication a(argc, argv);
	QDir::setCurrent(a.applicationDirPath());
	QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);

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
