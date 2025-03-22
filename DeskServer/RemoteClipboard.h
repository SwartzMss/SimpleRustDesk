#ifndef REMOTECLIPBOARD_H
#define REMOTECLIPBOARD_H

#include <QObject>
#include "rendezvous.pb.h"

class RemoteClipboard : public QObject {
	Q_OBJECT
public:
	explicit RemoteClipboard(QObject* parent = nullptr);

public slots:
	// 接收远程传来的 ClipboardEvent 消息，并更新系统剪贴板数据
	void onClipboardMessageReceived(const ClipboardEvent& clipboardEvent);
};

#endif // REMOTECLIPBOARD_H
