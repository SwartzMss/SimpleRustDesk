#include "VideoReceiver.h"
#include "NetworkWorker.h"
#include "VideoDecoderWorker.h"
#include "LogWidget.h"

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
{
    // 1) 创建线程
    m_networkThread = new QThread(this);
    m_decodeThread = new QThread(this);

    // 2) 创建两个 Worker，但不指定 parent（后面 moveToThread）
    m_netWorker = new NetworkWorker();        // 负责 TCP 网络收包
    m_decoderWorker = new VideoDecoderWorker();  // 负责解码

    // 3) 移动到各自的线程
    m_netWorker->moveToThread(m_networkThread);
    m_decoderWorker->moveToThread(m_decodeThread);

    // 4) 线程结束时自动清理 Worker
    connect(m_networkThread, &QThread::finished, m_netWorker, &QObject::deleteLater);
    connect(m_decodeThread, &QThread::finished, m_decoderWorker, &QObject::deleteLater);

    // 5) 信号槽连接
    // 当网络线程拆完一包数据，就发给解码线程
    connect(m_netWorker, &NetworkWorker::packetReady,
        m_decoderWorker, &VideoDecoderWorker::decodePacket,
        Qt::QueuedConnection);

    // 解码完成后回到主线程
    connect(m_decoderWorker, &VideoDecoderWorker::frameDecoded,
        this, &VideoReceiver::onFrameDecoded,
        Qt::QueuedConnection);

    // 网络出错 -> 通知本类
    connect(m_netWorker, &NetworkWorker::networkError,
        this, &VideoReceiver::onNetworkError,
        Qt::QueuedConnection);

    // 启动线程，让它们的事件循环开始工作
    m_networkThread->start();
    m_decodeThread->start();
}

VideoReceiver::~VideoReceiver()
{
    stopReceiving();
}

void VideoReceiver::stopReceiving()
{
	if (m_stopped)
		return;
	QMetaObject::invokeMethod(m_netWorker, "cleanup", Qt::QueuedConnection);
	QMetaObject::invokeMethod(m_decoderWorker, "cleanup", Qt::QueuedConnection);
	m_networkThread->quit();
	m_decodeThread->quit();
	m_networkThread->wait();
	m_decodeThread->wait();
    m_stopped = true;
}

void VideoReceiver::startConnect(const QString& host, quint16 port, const QString& uuid)
{
    // 主线程里只要 调用 Worker 的 connectToServer，即可让网络线程连
    // 这里需要使用 invokeMethod 或 queued signal 来跨线程调用
    QMetaObject::invokeMethod(m_netWorker,
        "connectToServer",
        Qt::QueuedConnection,
        Q_ARG(QString, host),
        Q_ARG(quint16, port),
        Q_ARG(QString, uuid));
    m_stopped = false;
}

void VideoReceiver::onFrameDecoded(const QImage& img)
{
    emit frameReady(img);
}

void VideoReceiver::onNetworkError(const QString& err)
{
    LogWidget::instance()->addLog("Network error: " + err, LogWidget::Warning);
    emit networkError(err);
}

void VideoReceiver::mouseEventCaptured(int x, int y, int mask)
{
	QMetaObject::invokeMethod(m_netWorker,
		"sendMouseEventToServer",
		Qt::QueuedConnection,
		Q_ARG(int, x),
		Q_ARG(int, y),
		Q_ARG(int, mask));
}
