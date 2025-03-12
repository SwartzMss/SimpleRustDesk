#ifndef VIDEODECODERWORKER_H
#define VIDEODECODERWORKER_H

#include <QObject>
#include <QByteArray>
#include <QImage>

// FFmpeg 相关头文件
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class VideoDecoderWorker : public QObject {
	Q_OBJECT
public:
	explicit VideoDecoderWorker(QObject* parent = nullptr);
	~VideoDecoderWorker();

public slots:
	// 【MOD】用于接收每个 H264 码流包并进行解码
	void decodePacket(const QByteArray& packetData);

signals:
	// 【MOD】工作线程里解码完成后，通知外部得到图像帧
	void frameDecoded(const QImage& image);

private:
	const AVCodec* codec = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	SwsContext* swsCtx = nullptr;
};

#endif // VIDEODECODERWORKER_H
