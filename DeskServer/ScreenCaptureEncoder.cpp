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

	// 查找 H264 编码器
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		qFatal("H264 codec not found");
	}

	// 分配编码上下文
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		qFatal("Could not allocate video codec context");
	}

	codecCtx->bit_rate = screenSize.width() * screenSize.height() * 4; // 根据分辨率调整比特率
	codecCtx->width = screenSize.width();
	codecCtx->height = screenSize.height();
	codecCtx->time_base = AVRational{ 1, 30 }; // 30 fps
	codecCtx->framerate = AVRational{ 30, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// 设置低延迟预设（编码器需支持该选项）
	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);

	// 打开编码器
	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		qFatal("Could not open codec");
	}

	// 分配视频帧
	frame = av_frame_alloc();
	if (!frame) {
		qFatal("Could not allocate video frame");
	}
	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;

	// 分配帧缓冲区
	int ret = av_image_alloc(frame->data, frame->linesize, codecCtx->width, codecCtx->height,
		codecCtx->pix_fmt, 32);
	if (ret < 0) {
		qFatal("Could not allocate raw picture buffer");
	}

	// 初始化转换上下文，从 QImage 的 BGRA 转换为 YUV420P
	swsCtx = sws_getContext(codecCtx->width, codecCtx->height, AV_PIX_FMT_BGRA,
		codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (!swsCtx) {
		qFatal("Could not initialize the conversion context");
	}

	// 创建定时器，实现约30fps的捕获频率
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


// 当检测到屏幕分辨率变化时，重新初始化编码器
void ScreenCaptureEncoder::reinitializeEncoder(int newWidth, int newHeight)
{
	// 停止定时器，防止并发访问
	timer->stop();

	// 释放之前的资源
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

	// 重新分配编码上下文
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		qFatal("Could not allocate video codec context");
	}

	// 根据新分辨率调整比特率与其它参数
	codecCtx->bit_rate = newWidth * newHeight * 4;
	codecCtx->width = newWidth;
	codecCtx->height = newHeight;
	codecCtx->time_base = AVRational{ 1, 30 };
	codecCtx->framerate = AVRational{ 30, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		qFatal("Could not open codec");
	}

	// 分配新的视频帧
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

	// 重新初始化转换上下文
	swsCtx = sws_getContext(codecCtx->width, codecCtx->height, AV_PIX_FMT_BGRA,
		codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (!swsCtx) {
		qFatal("Could not initialize the conversion context");
	}

	qDebug() << "Encoder reinitialized with resolution:" << newWidth << "x" << newHeight;

	// 重启定时器
	timer->start(33);
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

	// 检测当前屏幕尺寸
	QSize currentScreenSize = screen->size();
	if (currentScreenSize.width() != codecCtx->width || currentScreenSize.height() != codecCtx->height) {
		qDebug() << "Screen resolution changed from" << codecCtx->width << "x" << codecCtx->height
			<< "to" << currentScreenSize.width() << "x" << currentScreenSize.height();
		// 分辨率变化时重新初始化编码器
		reinitializeEncoder(currentScreenSize.width(), currentScreenSize.height());
		return; // 本次不进行编码，下个周期会使用新参数捕获
	}

	// 捕获整个屏幕
	QPixmap pixmap = screen->grabWindow(0);
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

	// 如果需要按比例缩放（这里保持比例），可以调整为合适的策略
	QImage scaledImage = image.scaled(codecCtx->width, codecCtx->height, Qt::KeepAspectRatio);

	uint8_t* srcData[4] = { scaledImage.bits(), nullptr, nullptr, nullptr };
	int srcLinesize[4] = { static_cast<int>(scaledImage.bytesPerLine()), 0, 0, 0 };

	// 转换图像格式
	sws_scale(swsCtx, srcData, srcLinesize, 0, codecCtx->height, frame->data, frame->linesize);

	frame->pts = frameCounter++;

	// 编码该帧
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
