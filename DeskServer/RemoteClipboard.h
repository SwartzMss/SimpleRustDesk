#ifndef REMOTECLIPBOARD_H
#define REMOTECLIPBOARD_H

#include <QObject>
#include "rendezvous.pb.h"

class RemoteClipboard : public QObject {
	Q_OBJECT
public:
	explicit RemoteClipboard(QObject* parent = nullptr);

public slots:
	// ����Զ�̴����� ClipboardEvent ��Ϣ��������ϵͳ����������
	void onClipboardMessageReceived(const ClipboardEvent& clipboardEvent);
};

#endif // REMOTECLIPBOARD_H
