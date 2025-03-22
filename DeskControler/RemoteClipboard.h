#ifndef REMOTECLIPBOARD_H
#define REMOTECLIPBOARD_H

#include <QObject>
#include <windows.h>
#include "rendezvous.pb.h"

Q_DECLARE_METATYPE(ClipboardEvent) 

class RemoteClipboard : public QObject {
	Q_OBJECT
public:
	explicit RemoteClipboard(QObject* parent = nullptr);
	~RemoteClipboard();

	bool start();
	void stop();

	void setRemoteWindow(QWidget* remoteWindow);
signals:
	void ctrlCPressed(const ClipboardEvent& clipboardEvent);

private:
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT handleKeyEvent(int nCode, WPARAM wParam, LPARAM lParam);

	static HHOOK s_hook;
	static RemoteClipboard* s_instance;
	QWidget* m_remoteWindow = nullptr;
};

#endif // REMOTECLIPBOARD_H
