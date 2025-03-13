#ifndef SCREENCAPTUREENCODER_H
#define SCREENCAPTUREENCODER_H

#include <QObject>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QDebug>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class ScreenCaptureEncoder : public QObject {
	Q_OBJECT
public:
	explicit ScreenCaptureEncoder(QObject* parent = nullptr);
	~ScreenCaptureEncoder();

	void startCapture();
	// ֹͣ����
	void stopCapture();

signals:
	// ����������ݰ��󷢳��źţ����ⲿ�������߼�
	void encodedPacketReady(const QByteArray& packet);

private slots:
	void captureAndEncode();

private:
	void reinitializeEncoder(int newWidth, int newHeight);
private:
	// FFmpeg��س�Ա
	const AVCodec* codec;
	AVCodecContext* codecCtx;
	AVFrame* frame;
	struct SwsContext* swsCtx;
	int frameCounter;
	QTimer* timer;
};

#endif // SCREENCAPTUREENCODER_H
