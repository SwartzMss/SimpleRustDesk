#include "VideoDecoderWorker.h"
#include "LogWidget.h"   // ������־���

VideoDecoderWorker::VideoDecoderWorker(QObject* parent)
	: QObject(parent)
{
	// FFmpeg ��ʼ��
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		LogWidget::instance()->addLog(QString("H264 decoder not found"), LogWidget::Error);
		return;
	}
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		LogWidget::instance()->addLog(QString("Could not allocate video codec context"), LogWidget::Error);
		return;
	}
	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		LogWidget::instance()->addLog(QString("Could not open codec"), LogWidget::Error);
		return;
	}
	frame = av_frame_alloc();
	if (!frame) {
		LogWidget::instance()->addLog(QString("Could not allocate video frame"), LogWidget::Error);
	}
}

VideoDecoderWorker::~VideoDecoderWorker()
{
	if (swsCtx) {
		sws_freeContext(swsCtx);
		swsCtx = nullptr;
	}
	if (frame) {
		av_frame_free(&frame);
		frame = nullptr;
	}
	if (codecCtx) {
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;
	}
}

void VideoDecoderWorker::decodePacket(const QByteArray& packetData)
{

	LogWidget::instance()->addLog(QString("decodePacket"), LogWidget::Info);
	return;
	// �� packetData ������ AVPacket
	AVPacket* pkt = av_packet_alloc();
	if (!pkt) {
		LogWidget::instance()->addLog(QString("Could not allocate AVPacket"), LogWidget::Warning);
		return;
	}
	pkt->data = reinterpret_cast<uint8_t*>(const_cast<char*>(packetData.data()));
	pkt->size = packetData.size();

	int ret = avcodec_send_packet(codecCtx, pkt);
	if (ret < 0) {
		LogWidget::instance()->addLog(QString("Error sending packet for decoding"), LogWidget::Warning);
		av_packet_free(&pkt);
		return;
	}

	// ���Խ������н������֡
	while (true) {
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			break;
		}
		else if (ret < 0) {
			LogWidget::instance()->addLog(QString("Error during decoding"), LogWidget::Warning);
			break;
		}
		// �õ�����֡��YUV420P��

		// ��ʼ��ת�������ģ������û������
		if (!swsCtx) {
			swsCtx = sws_getContext(frame->width, frame->height, codecCtx->pix_fmt,
				frame->width, frame->height, AV_PIX_FMT_RGBA,
				SWS_BILINEAR, nullptr, nullptr, nullptr);
		}
		// ����Ŀ�껺������С
		int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
		QByteArray imgBuffer(numBytes, 0);

		uint8_t* destData[4] = {
			reinterpret_cast<uint8_t*>(imgBuffer.data()),
			nullptr, nullptr, nullptr
		};
		int destLinesize[4] = { 4 * frame->width, 0, 0, 0 };

		// ת��Ϊ RGBA
		sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
			destData, destLinesize);

		// ���� QImage
		QImage image(reinterpret_cast<const uchar*>(imgBuffer.constData()),
			frame->width, frame->height, QImage::Format_RGBA8888);
		// ����һ�ݣ���ֹ�������ݱ�����
		QImage finalImage = image.copy();

		// �����źţ�֪ͨ�ⲿ��֡�ѽ���
		emit frameDecoded(finalImage);
	}

	av_packet_free(&pkt);
}
