#include "ScreenCaptureEncoder.h"

ScreenCaptureEncoder::ScreenCaptureEncoder(QObject* parent)
	: QObject(parent), codec(nullptr), codecCtx(nullptr), frame(nullptr),
	swsCtx(nullptr), frameCounter(0)
{

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

	// 设置编码参数（可根据需要调整）
	codecCtx->bit_rate = 400000;
	codecCtx->width = 1280;      // 目标宽度
	codecCtx->height = 720;      // 目标高度
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

void ScreenCaptureEncoder::captureAndEncode()
{
	// 使用 QScreen 捕获整个屏幕
	QScreen* screen = QGuiApplication::primaryScreen();
	if (!screen) {
		qWarning() << "No primary screen found!";
		return;
	}
	QPixmap pixmap = screen->grabWindow(0);
	QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);

	// 调整图像大小以匹配编码目标分辨率
	QImage scaledImage = image.scaled(codecCtx->width, codecCtx->height, Qt::KeepAspectRatio);

	// 将 QImage 数据传递给 sws_scale 进行格式转换
	uint8_t* srcData[4] = { scaledImage.bits(), nullptr, nullptr, nullptr };
	int srcLinesize[4] = { static_cast<int>(scaledImage.bytesPerLine()), 0, 0, 0 };

	// 转换为 YUV420P 格式
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
	ret = avcodec_receive_packet(codecCtx, pkt);  // 直接传入 pkt，而不是 &pkt
	if (ret == 0) {
		qDebug() << "Encoded packet size:" << pkt->size;
		// 将编码后的数据包转换为 QByteArray，并通过信号传递给外部处理
		QByteArray data(reinterpret_cast<const char*>(pkt->data), pkt->size);
		emit encodedPacketReady(data);
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		// 当前无可用数据，记得释放 pkt
		av_packet_free(&pkt);
	}
	else {
		qWarning() << "Error during encoding";
		av_packet_free(&pkt);
	}
}
