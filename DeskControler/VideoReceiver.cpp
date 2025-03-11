#include "VideoReceiver.h"
#include <QDebug>
#include <QtEndian>  // ���� qFromBigEndian

VideoReceiver::VideoReceiver(QObject* parent)
	: QObject(parent), socket(nullptr), codec(nullptr), codecCtx(nullptr),
	frame(nullptr), swsCtx(nullptr)
{
	// ���� H264 ������
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		qFatal("H264 decoder not found");
	}
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		qFatal("Could not allocate video codec context");
	}
	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		qFatal("Could not open codec");
	}
	frame = av_frame_alloc();
	if (!frame) {
		qFatal("Could not allocate video frame");
	}
}

VideoReceiver::~VideoReceiver()
{
	if (socket) {
		socket->disconnectFromHost();
		socket->deleteLater();
	}
	if (swsCtx)
		sws_freeContext(swsCtx);
	if (frame)
		av_frame_free(&frame);
	if (codecCtx)
		avcodec_free_context(&codecCtx);
}

void VideoReceiver::connectToServer(const QString& host, quint16 port)
{
	socket = new QTcpSocket(this);
	connect(socket, &QTcpSocket::readyRead, this, &VideoReceiver::onSocketReadyRead);
	connect(socket, &QTcpSocket::errorOccurred,
		this, &VideoReceiver::onSocketError);
	socket->connectToHost(host, port);
}

void VideoReceiver::onSocketReadyRead()
{
	// ��ȡ�������ݲ�׷�ӵ� buffer ��
	buffer.append(socket->readAll());

	// Э��Լ�������ݰ���ʽΪ [4�ֽ����������] + [������]
	while (buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);
		if (buffer.size() < 4 + static_cast<int>(packetSize))
			break; // ���ݲ������ȴ���������

		QByteArray packetData = buffer.mid(4, packetSize);
		buffer.remove(0, 4 + packetSize);

		// ʹ�� FFmpeg ���н��룬�滻 av_init_packet
		AVPacket* pkt = av_packet_alloc();
		if (!pkt) {
			qWarning() << "Could not allocate AVPacket";
			continue;
		}
		pkt->data = reinterpret_cast<uint8_t*>(packetData.data());
		pkt->size = packetData.size();

		int ret = avcodec_send_packet(codecCtx, pkt);
		if (ret < 0) {
			qWarning() << "Error sending packet for decoding";
			av_packet_free(&pkt);
			continue;
		}
		// ���Խ������н������֡
		while (true) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				qWarning() << "Error during decoding";
				break;
			}
			// �õ�����֡����ʱ��ʽΪ YUV420P

			// ��ʼ��ת�������ģ������û�������� YUV420P תΪ RGBA ��ʽ
			if (!swsCtx) {
				swsCtx = sws_getContext(frame->width, frame->height, codecCtx->pix_fmt,
					frame->width, frame->height, AV_PIX_FMT_RGBA,
					SWS_BILINEAR, nullptr, nullptr, nullptr);
			}
			// ����Ŀ�껺������С
			int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
			QByteArray imgBuffer(numBytes, 0);
			uint8_t* destData[4] = { reinterpret_cast<uint8_t*>(imgBuffer.data()), nullptr, nullptr, nullptr };
			int destLinesize[4] = { 4 * frame->width, 0, 0, 0 };

			// ת��Ϊ RGBA ��ʽ
			sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);

			// ���� QImage���˴� Format_RGBA8888 �� RGBA ��ʽ��Ӧ
			QImage image(reinterpret_cast<const uchar*>(imgBuffer.constData()),
				frame->width, frame->height, QImage::Format_RGBA8888);
			// ����һ�ݣ�ȷ���ڴ氲ȫ
			QImage finalImage = image.copy();
			emit frameReady(finalImage);
		}
		av_packet_free(&pkt);
	}
}

void VideoReceiver::onSocketError(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error);
	qWarning() << "Socket error:" << socket->errorString();
}
