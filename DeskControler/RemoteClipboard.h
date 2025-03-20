#ifndef REMOTECLIPBOARD_H
#define REMOTECLIPBOARD_H

#include <QObject>
#include <QEvent>
#include <QMimeData>
#include "rendezvous.pb.h"

class RemoteClipboard : public QObject {
	Q_OBJECT
public:
	explicit RemoteClipboard(QObject* parent = nullptr);

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	void sendClipboardData(const QMimeData* mimeData);


signals:
	void clipboardDataReady(const ClipboardEvent& clipboardEvent);
};

#endif // REMOTECLIPBOARD_H
