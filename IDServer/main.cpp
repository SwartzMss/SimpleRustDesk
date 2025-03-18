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
	// 定义 dump 文件名，建议使用动态命名以免覆盖之前的 dump 文件
	TCHAR dumpFileName[MAX_PATH] = _T("crash_dump.dmp");

	// 创建 dump 文件
	HANDLE hFile = CreateFile(dumpFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		// 设置 dump 参数
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = pExceptionPointers;
		dumpInfo.ClientPointers = FALSE;

		// 生成 dump 文件，MiniDumpNormal 生成最基本的信息
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

	// 返回 EXCEPTION_EXECUTE_HANDLER，系统将终止程序
	return EXCEPTION_EXECUTE_HANDLER;
}


bool checkSingleInstance(const QString& key) {
	static QSharedMemory sharedMem(key);
	if (!sharedMem.create(1)) {
		return false; // 已有实例在运行
	}
	return true; // 没有实例在运行
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

	// 禁止最大化
	w.setWindowFlags(w.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    w.show();
    return a.exec();
}
