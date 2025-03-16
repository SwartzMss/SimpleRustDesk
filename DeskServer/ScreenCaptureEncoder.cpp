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

	// MOD: 降低比特率，从原来的 width*height*4 调整为 width*height*2
	codecCtx->bit_rate = screenSize.width() * screenSize.height() * 2;
	codecCtx->width = screenSize.width();
	codecCtx->height = screenSize.height();
	// MOD: 降低帧率到20fps（原来30fps）
	codecCtx->time_base = AVRational{ 1, 20 };
	codecCtx->framerate = AVRational{ 20, 1 };
	codecCtx->gop_size = 10;
	codecCtx->max_b_frames = 1;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// 设置低延迟预设和零延迟调优
	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);
	// MOD: 增加零延迟调优选项
	av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);

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

	// MOD: 调整定时器间隔，匹配20fps（50ms每帧）
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
	// MOD: 使用50ms间隔以实现20fps
	timer->start(50);
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

	// MOD: 使用新的分辨率和调优参数（保持原分辨率不变）
	QSize newSize(newWidth, newHeight);
	codecCtx->bit_rate = newSize.width() * newSize.height() * 2; // MOD: bit_rate调整
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

	qDebug() << "Encoder reinitialized with resolution:" << codecCtx->width << "x" << codecCtx->height;

	// MOD: 重启定时器，使用50ms间隔
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

	// MOD: 直接使用原始分辨率，不改变尺寸
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
