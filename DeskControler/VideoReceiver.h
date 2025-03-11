#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QByteArray>
#include <QImage>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class VideoReceiver : public QObject {
	Q_OBJECT
public:
	explicit VideoReceiver(QObject* parent = nullptr);
	~VideoReceiver();

	// 连接到服务器（例如 "127.0.0.1", 12345）
	void connectToServer(const QString& host, quint16 port);

signals:
	// 当解码出一帧图像时发出信号
	void frameReady(const QImage& image);

private slots:
	void onSocketReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);

private:
	QTcpSocket* socket;
	QByteArray buffer; // 用于存储接收的数据

	// FFmpeg 解码相关成员
	const AVCodec* codec;
	AVCodecContext* codecCtx;
	AVFrame* frame;
	// 转换上下文，将 YUV420P 转为 RGBA（QImage 使用的格式）
	SwsContext* swsCtx;
};

#endif // VIDEORECEIVER_H
