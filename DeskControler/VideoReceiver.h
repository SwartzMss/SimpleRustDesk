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

	// ���ӵ������������� "127.0.0.1", 12345��
	void connectToServer(const QString& host, quint16 port);

signals:
	// �������һ֡ͼ��ʱ�����ź�
	void frameReady(const QImage& image);

private slots:
	void onSocketReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);

private:
	QTcpSocket* socket;
	QByteArray buffer; // ���ڴ洢���յ�����

	// FFmpeg ������س�Ա
	const AVCodec* codec;
	AVCodecContext* codecCtx;
	AVFrame* frame;
	// ת�������ģ��� YUV420P תΪ RGBA��QImage ʹ�õĸ�ʽ��
	SwsContext* swsCtx;
};

#endif // VIDEORECEIVER_H
