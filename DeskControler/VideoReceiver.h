#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QByteArray>
#include <QImage>
#include <QUuid>
#include <QThread>
#include "VideoDecoderWorker.h"  // 【MOD】 引入独立的 Worker 头文件

class VideoReceiver : public QObject {
	Q_OBJECT
public:
	explicit VideoReceiver(QObject* parent = nullptr);
	~VideoReceiver();

	void connectToServer(const QString& host, quint16 port, const QString& uuid);

signals:
	void frameReady(const QImage& image);

private slots:
	void onSocketConnected();
	void onSocketReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);

	// 【MOD】工作线程解码好帧后，交给本类的槽，再转发 frameReady
	void onFrameDecoded(const QImage& image);

private:
	QTcpSocket* socket = nullptr;
	QByteArray buffer;
	QString relayUuid;

	// 【MOD】新增：线程与 Worker
	QThread* m_decodeThread = nullptr;
	VideoDecoderWorker* m_worker = nullptr;
};

#endif // VIDEORECEIVER_H
