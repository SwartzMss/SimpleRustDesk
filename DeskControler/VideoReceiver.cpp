#include "VideoReceiver.h"
#include "LogWidget.h"
#include <QtEndian>
#include "rendezvous.pb.h"

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent),
    socket(nullptr)
{
    // 【MOD】创建解码线程 & Worker
    m_decodeThread = new QThread(this);
    m_worker = new VideoDecoderWorker();     // 无父对象，后面手动 moveToThread
    m_worker->moveToThread(m_decodeThread);

    // 线程结束自动清理 Worker
    connect(m_decodeThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 连接 Worker 的解码完成信号到本类的 onFrameDecoded
    connect(m_worker, &VideoDecoderWorker::frameDecoded,
        this, &VideoReceiver::onFrameDecoded);

    m_decodeThread->start();
}

VideoReceiver::~VideoReceiver()
{
    if (m_decodeThread) {
        m_decodeThread->quit();
        m_decodeThread->wait();  // 等待线程真正退出
    }
    if (socket) {
        socket->disconnectFromHost();
        socket->deleteLater();
        socket = nullptr;
    }
}

void VideoReceiver::connectToServer(const QString& host, quint16 port, const QString& uuid)
{
    relayUuid = uuid;
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &VideoReceiver::onSocketConnected);
    connect(socket, &QTcpSocket::readyRead, this, &VideoReceiver::onSocketReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &VideoReceiver::onSocketError);

    socket->connectToHost(host, port);
}

void VideoReceiver::onSocketConnected()
{
    // 发送 RequestRelay 消息（保持原逻辑）
    RendezvousMessage msg;
    RequestRelay* req = msg.mutable_request_relay();
    req->set_uuid(relayUuid.toUtf8().constData(), relayUuid.toUtf8().size());

    std::string outStr;
    if (!msg.SerializeToString(&outStr)) {
        LogWidget::instance()->addLog(QString("Failed to serialize RequestRelay message"), LogWidget::Error);
        return;
    }
    QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
    socket->write(data);
    socket->flush();
}

void VideoReceiver::onSocketReadyRead()
{
    buffer.append(socket->readAll());

    int packetsProcessed = 0;               // 【MOD】新增计数
    const int MAX_PACKETS_PER_CYCLE = 10;   // 每轮最多处理 10 个包，随需求调整

    while (buffer.size() >= 4) {
        // 如果这次已经处理了 10 个包，就让出主线程，等下一次事件循环
        if (packetsProcessed >= MAX_PACKETS_PER_CYCLE) {
            break;
        }

        quint32 packetSize;
        memcpy(&packetSize, buffer.constData(), 4);
        packetSize = qFromBigEndian(packetSize);
        if (buffer.size() < 4 + static_cast<int>(packetSize)) {
            break;
        }

        QByteArray packetData = buffer.mid(4, packetSize);
        buffer.remove(0, 4 + packetSize);

        // 投递给解码线程
        QMetaObject::invokeMethod(
            m_worker,
            "decodePacket",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, packetData)
        );

        packetsProcessed++;
    }

    // 如果还有剩余数据没有处理完，会在下一次事件循环再次触发 readyRead
    // 因为 QTcpSocket 在 buffer 里仍有可读数据，也可主动调用
    // QMetaObject::invokeMethod(this, "onSocketReadyRead", Qt::QueuedConnection);
    // 以便马上继续处理。但通常不必手动调用，只要还有数据可读，
    // Qt 会自动再次发出 readyRead 或在下一次事件循环调度。
}


void VideoReceiver::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    LogWidget::instance()->addLog(QString("onSocketError %1").arg(socket->errorString()), LogWidget::Warning);
}

// 【MOD】当工作线程解码出图像时调用
void VideoReceiver::onFrameDecoded(const QImage& image)
{
    // 这里转发成对外可用的 signal，供 UI 使用
    emit frameReady(image);
}
