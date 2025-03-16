#include "ScreenCaptureEncoder.h"

ScreenCaptureEncoder::ScreenCaptureEncoder(QObject* parent)
	: QObject(parent), codec(nullptr), codecCtx(nullptr), frame(nullptr),
	swsCtx(nullptr), frameCounter(0)
{
	QScreen* screen = QGuiApplication::primaryScreen();
	if (!screen) {
		qFatal("No primary screen found!");
	}
	QSize screenSize = screen->size();

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

	// MOD: ���ͱ����ʣ���ԭ���� width*height*4 ����Ϊ width*height*2
	codecCtx->bit_rate = screenSize.width() * screenSize.height() * 2;
	codecCtx->width = screenSize.width();
	codecCtx->height = screenSize.height();
	// MOD: ����֡�ʵ�20fps��ԭ��30fps��
	codecCtx->time_base = AVRational{ 1, 20 };
	codecCtx->framerate = AVRational{ 20, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// ���õ��ӳ�Ԥ������ӳٵ���
	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);
	// MOD: �������ӳٵ���ѡ��
	av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);

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

	// MOD: ������ʱ�������ƥ��20fps��50msÿ֡��
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
	// MOD: ʹ��50ms�����ʵ��20fps
	timer->start(50);
}


// ����⵽��Ļ�ֱ��ʱ仯ʱ�����³�ʼ��������
void ScreenCaptureEncoder::reinitializeEncoder(int newWidth, int newHeight)
{
	// ֹͣ��ʱ������ֹ��������
	timer->stop();

	// �ͷ�֮ǰ����Դ
	if (swsCtx) {
		sws_freeContext(swsCtx);
		swsCtx = nullptr;
	}
	if (frame) {
		av_freep(&frame->data[0]);
		av_frame_free(&frame);
		frame = nullptr;
	}
	if (codecCtx) {
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;
	}

	// ���·������������
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		qFatal("Could not allocate video codec context");
	}

	// MOD: ʹ���µķֱ��ʺ͵��Ų���������ԭ�ֱ��ʲ��䣩
	QSize newSize(newWidth, newHeight);
	codecCtx->bit_rate = newSize.width() * newSize.height() * 2; // MOD: bit_rate����
	codecCtx->width = newSize.width();
	codecCtx->height = newSize.height();
	codecCtx->time_base = AVRational{ 1, 20 }; // MOD: 20fps
	codecCtx->framerate = AVRational{ 20, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		qFatal("Could not open codec");
	}

	// �����µ���Ƶ֡
	frame = av_frame_alloc();
	if (!frame) {
		qFatal("Could not allocate video frame");
	}
	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;

	int ret = av_image_alloc(frame->data, frame->linesize, codecCtx->width, codecCtx->height,
		codecCtx->pix_fmt, 32);
	if (ret < 0) {
		qFatal("Could not allocate raw picture buffer");
	}

	// ���³�ʼ��ת��������
	swsCtx = sws_getContext(codecCtx->width, codecCtx->height, AV_PIX_FMT_BGRA,
		codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (!swsCtx) {
		qFatal("Could not initialize the conversion context");
	}

	qDebug() << "Encoder reinitialized with resolution:" << codecCtx->width << "x" << codecCtx->height;

	// MOD: ������ʱ����ʹ��50ms���
	timer->start(50);
}


void ScreenCaptureEncoder::stopCapture()
{
	if (timer) {
		timer->stop();
	}
}

void ScreenCaptureEncoder::captureAndEncode()
{
	QScreen* screen = QGuiApplication::primaryScreen();
	if (!screen) {
		qWarning() << "No primary screen found!";
		return;
	}

	// ��⵱ǰ��Ļ�ߴ�
	QSize currentScreenSize = screen->size();
	if (currentScreenSize.width() != codecCtx->width || currentScreenSize.height() != codecCtx->height) {
		qDebug() << "Screen resolution changed from" << codecCtx->width << "x" << codecCtx->height
			<< "to" << currentScreenSize.width() << "x" << currentScreenSize.height();
		// �ֱ��ʱ仯ʱ���³�ʼ��������
		reinitializeEncoder(currentScreenSize.width(), currentScreenSize.height());
		return; // ���β����б��룬�¸����ڻ�ʹ���²�������
	}

	// ����������Ļ
	QPixmap pixmap = screen->grabWindow(0);
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

	// MOD: ֱ��ʹ��ԭʼ�ֱ��ʣ����ı�ߴ�
	QImage scaledImage = image.scaled(codecCtx->width, codecCtx->height, Qt::KeepAspectRatio);

	uint8_t* srcData[4] = { scaledImage.bits(), nullptr, nullptr, nullptr };
	int srcLinesize[4] = { static_cast<int>(scaledImage.bytesPerLine()), 0, 0, 0 };

	// ת��ͼ���ʽ
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
	ret = avcodec_receive_packet(codecCtx, pkt);
	if (ret == 0) {
		qDebug() << "Encoded packet size:" << pkt->size;
		QByteArray data(reinterpret_cast<const char*>(pkt->data), pkt->size);
		emit encodedPacketReady(data);
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		av_packet_free(&pkt);
	}
	else {
		qWarning() << "Error during encoding";
		av_packet_free(&pkt);
	}
}
