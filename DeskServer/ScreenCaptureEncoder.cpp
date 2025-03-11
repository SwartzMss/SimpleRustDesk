#include "ScreenCaptureEncoder.h"

ScreenCaptureEncoder::ScreenCaptureEncoder(QObject* parent)
	: QObject(parent), codec(nullptr), codecCtx(nullptr), frame(nullptr),
	swsCtx(nullptr), frameCounter(0)
{

	// ���� H264 ������
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		qFatal("H264 codec not found");
	}

	// �������������
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		qFatal("Could not allocate video codec context");
	}

	// ���ñ���������ɸ�����Ҫ������
	codecCtx->bit_rate = 400000;
	codecCtx->width = 1280;      // Ŀ����
	codecCtx->height = 720;      // Ŀ��߶�
	codecCtx->time_base = AVRational{ 1, 30 }; // 30 fps
	codecCtx->framerate = AVRational{ 30, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// ���õ��ӳ�Ԥ�裨��������֧�ָ�ѡ�
	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);

	// �򿪱�����
	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		qFatal("Could not open codec");
	}

	// ������Ƶ֡
	frame = av_frame_alloc();
	if (!frame) {
		qFatal("Could not allocate video frame");
	}
	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;

	// ����֡������
	int ret = av_image_alloc(frame->data, frame->linesize, codecCtx->width, codecCtx->height,
		codecCtx->pix_fmt, 32);
	if (ret < 0) {
		qFatal("Could not allocate raw picture buffer");
	}

	// ��ʼ��ת�������ģ��� QImage �� BGRA ת��Ϊ YUV420P
	swsCtx = sws_getContext(codecCtx->width, codecCtx->height, AV_PIX_FMT_BGRA,
		codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (!swsCtx) {
		qFatal("Could not initialize the conversion context");
	}

	// ������ʱ����ʵ��Լ30fps�Ĳ���Ƶ��
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &ScreenCaptureEncoder::captureAndEncode);
}

ScreenCaptureEncoder::~ScreenCaptureEncoder()
{
	if (timer) timer->stop();
	if (swsCtx)
		sws_freeContext(swsCtx);
	if (frame) {
		av_freep(&frame->data[0]);
		av_frame_free(&frame);
	}
	if (codecCtx) {
		avcodec_free_context(&codecCtx);
	}
}

void ScreenCaptureEncoder::startCapture()
{
	timer->start(33);
}

void ScreenCaptureEncoder::captureAndEncode()
{
	// ʹ�� QScreen ����������Ļ
	QScreen* screen = QGuiApplication::primaryScreen();
	if (!screen) {
		qWarning() << "No primary screen found!";
		return;
	}
	QPixmap pixmap = screen->grabWindow(0);
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

	// ����ͼ���С��ƥ�����Ŀ��ֱ���
	QImage scaledImage = image.scaled(codecCtx->width, codecCtx->height, Qt::KeepAspectRatio);

	// �� QImage ���ݴ��ݸ� sws_scale ���и�ʽת��
	uint8_t* srcData[4] = { scaledImage.bits(), nullptr, nullptr, nullptr };
	int srcLinesize[4] = { static_cast<int>(scaledImage.bytesPerLine()), 0, 0, 0 };

	// ת��Ϊ YUV420P ��ʽ
	sws_scale(swsCtx, srcData, srcLinesize, 0, codecCtx->height, frame->data, frame->linesize);

	frame->pts = frameCounter++;

	// �����֡
	AVPacket* pkt = av_packet_alloc();
	if (!pkt) {
		qWarning() << "Could not allocate AVPacket";
		return;
	}

	int ret = avcodec_send_frame(codecCtx, frame);
	if (ret < 0) {
		qWarning() << "Error sending frame for encoding";
		av_packet_free(&pkt);
		return;
	}
	ret = avcodec_receive_packet(codecCtx, pkt);  // ֱ�Ӵ��� pkt�������� &pkt
	if (ret == 0) {
		qDebug() << "Encoded packet size:" << pkt->size;
		// �����������ݰ�ת��Ϊ QByteArray����ͨ���źŴ��ݸ��ⲿ����
		QByteArray data(reinterpret_cast<const char*>(pkt->data), pkt->size);
		emit encodedPacketReady(data);
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		// ��ǰ�޿������ݣ��ǵ��ͷ� pkt
		av_packet_free(&pkt);
	}
	else {
		qWarning() << "Error during encoding";
		av_packet_free(&pkt);
	}
}
