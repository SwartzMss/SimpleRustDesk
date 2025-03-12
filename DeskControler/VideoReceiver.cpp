#include "VideoReceiver.h"
#include <QDebug>
#include <QtEndian>
#include "rendezvous.pb.h"  // 【MOD】包含 protobuf 消息定义

VideoReceiver::VideoReceiver(QObject* parent)
	: QObject(parent), socket(nullptr), codec(nullptr), codecCtx(nullptr),
	  frame(nullptr), swsCtx(nullptr)
{
	// 查找 H264 解码器
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

// 【MOD】修改 connectToServer，增加 uuid 参数，保存 uuid 并连接 relay 服务器
void VideoReceiver::connectToServer(const QString& host, quint16 port, const QString& uuid)
{
	relayUuid = uuid; // 保存用于 RequestRelay 的 uuid
	socket = new QTcpSocket(this);
	// 连接信号：连接成功、数据就绪、错误
	connect(socket, &QTcpSocket::connected, this, &VideoReceiver::onSocketConnected);
	connect(socket, &QTcpSocket::readyRead, this, &VideoReceiver::onSocketReadyRead);
	connect(socket, &QTcpSocket::errorOccurred, this, &VideoReceiver::onSocketError);
	socket->connectToHost(host, port);
}

// 【MOD】当连接建立后，构造并发送 RequestRelay 消息
void VideoReceiver::onSocketConnected()
{
	// 构造 RequestRelay 消息
	RendezvousMessage msg;
	RequestRelay* req = msg.mutable_request_relay();
	// 生成随机 id，并使用 relayUuid 作为 uuid
	QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	req->set_id(id.toUtf8().constData(), id.toUtf8().size());
	req->set_uuid(relayUuid.toUtf8().constData(), relayUuid.toUtf8().size());

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		qWarning() << "Failed to serialize RequestRelay message";
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
	socket->write(data);
	socket->flush();
	qDebug() << "Sent RequestRelay message with id:" << id << "uuid:" << relayUuid;
}

void VideoReceiver::onSocketReadyRead()
{
	// 读取所有数据并追加到 buffer 中
	buffer.append(socket->readAll());

	// 协议约定：数据包格式为 [4字节网络序包长] + [包数据]
	while (buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);
		if (buffer.size() < 4 + static_cast<int>(packetSize))
			break; // 数据不够，等待更多数据

		QByteArray packetData = buffer.mid(4, packetSize);
		buffer.remove(0, 4 + packetSize);

		// 使用 FFmpeg 进行解码
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
		// 尝试接收所有解码出的帧
		while (true) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				qWarning() << "Error during decoding";
				break;
			}
			// 得到解码帧，此时格式为 YUV420P

			// 初始化转换上下文（如果还没创建）将 YUV420P 转为 RGBA 格式
			if (!swsCtx) {
				swsCtx = sws_getContext(frame->width, frame->height, codecCtx->pix_fmt,
					frame->width, frame->height, AV_PIX_FMT_RGBA,
					SWS_BILINEAR, nullptr, nullptr, nullptr);
			}
			// 计算目标缓冲区大小
			int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
			QByteArray imgBuffer(numBytes, 0);
			uint8_t* destData[4] = { reinterpret_cast<uint8_t*>(imgBuffer.data()), nullptr, nullptr, nullptr };
			int destLinesize[4] = { 4 * frame->width, 0, 0, 0 };

			// 转换为 RGBA 格式
			sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, destData, destLinesize);

			// 构造 QImage，此处 Format_RGBA8888 与 RGBA 格式对应
			QImage image(reinterpret_cast<const uchar*>(imgBuffer.constData()),
				frame->width, frame->height, QImage::Format_RGBA8888);
			// 复制一份，确保内存安全
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
