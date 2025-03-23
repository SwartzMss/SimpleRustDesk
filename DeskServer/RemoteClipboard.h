#ifndef REMOTECLIPBOARD_H
#define REMOTECLIPBOARD_H

#include <QObject>
#include <windows.h>
#include "rendezvous.pb.h"

class RemoteClipboard : public QObject {
	Q_OBJECT
public:
	explicit RemoteClipboard(QObject* parent = nullptr);
	~RemoteClipboard();

	bool start();
	void stop();

signals:
	void ctrlCPressed(const ClipboardEvent& clipboardEvent);

public slots:
	// 接收远程传来的 ClipboardEvent 消息，并更新系统剪贴板数据
	void onClipboardMessageReceived(const ClipboardEvent& clipboardEvent);

private:
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT handleKeyEvent(int nCode, WPARAM wParam, LPARAM lParam);

	static HHOOK s_hook;
	static RemoteClipboard* s_instance;
};

#endif // REMOTECLIPBOARD_H