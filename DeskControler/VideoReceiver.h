#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#include <QThread>
#include <QImage>

class NetworkWorker;
class VideoDecoderWorker;

class VideoReceiver : public QObject
{
	Q_OBJECT
public:
	explicit VideoReceiver(QObject* parent = nullptr);
	~VideoReceiver();

	// 主线程调用，用于发起连接
	void startConnect(const QString& host, quint16 port, const QString& uuid);
	void stopReceiving();

signals:
	// 当成功解码一帧时，把图像发给外层（比如给 VideoWidget 显示）
	void frameReady(const QImage& image);
	// 可以把 NetworkWorker 的错误转发出去
	void networkError(const QString& error);

private slots:
	// 当解码线程发出 frameDecoded 时调用
	void onFrameDecoded(const QImage& img);
	// 当 NetworkWorker 报错时
	void onNetworkError(const QString& err);

private:
	QThread* m_networkThread = nullptr;
	QThread* m_decodeThread = nullptr;
	NetworkWorker* m_netWorker = nullptr;
	VideoDecoderWorker* m_decoderWorker = nullptr;
	bool m_stopped;
};

#endif // VIDEORECEIVER_H
