#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QByteArray>
#include <QImage>
#include <QUuid>  // 【MOD】用于生成 id

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
	// 【MOD】修改构造函数不变
	explicit VideoReceiver(QObject* parent = nullptr);
	~VideoReceiver();

	// 【MOD】重载 connectToServer，增加 uuid 参数，用于构造 RequestRelay 消息
	void connectToServer(const QString& host, quint16 port, const QString& uuid);

signals:
	// 当解码出一帧图像时发出信号
	void frameReady(const QImage& image);

private slots:
	void onSocketConnected(); // 【MOD】新增：连接建立后发送 RequestRelay 消息
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

	// 【MOD】保存发送 RequestRelay 消息时需要的 uuid
	QString relayUuid;
};

#endif // VIDEORECEIVER_H
