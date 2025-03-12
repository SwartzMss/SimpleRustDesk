#ifndef VIDEODECODERWORKER_H
#define VIDEODECODERWORKER_H

#include <QObject>
#include <QByteArray>
#include <QImage>

// FFmpeg ���ͷ�ļ�
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
	// ��MOD�����ڽ���ÿ�� H264 �����������н���
	void decodePacket(const QByteArray& packetData);

signals:
	// ��MOD�������߳��������ɺ�֪ͨ�ⲿ�õ�ͼ��֡
	void frameDecoded(const QImage& image);

private:
	const AVCodec* codec = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	SwsContext* swsCtx = nullptr;
};

#endif // VIDEODECODERWORKER_H
