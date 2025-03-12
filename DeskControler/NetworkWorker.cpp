#include "NetworkWorker.h"
#include "LogWidget.h"        // 如果你有日志功能
#include "rendezvous.pb.h"    // 假设你要发送 RequestRelay 消息

#include <QtEndian>

NetworkWorker::NetworkWorker(QObject* parent)
	: QObject(parent)
{
	// 不要在构造函数里创建 m_socket，后面在 connectToServer 里创建
}

NetworkWorker::~NetworkWorker()
{
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
	}
}

// 注意：这个槽会在工作线程里被调用
void NetworkWorker::connectToServer(const QString& host, quint16 port, const QString& uuid)
{
	m_uuid = uuid;  // 保存起来，用于后面发送 RequestRelay

	if (m_socket) {
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	m_socket = new QTcpSocket(this);

	// 连接 socket 信号到本类槽
	connect(m_socket, &QTcpSocket::connected, this, &NetworkWorker::onSocketConnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &NetworkWorker::onSocketReadyRead);
	connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkWorker::onSocketError);
	connect(m_socket, &QTcpSocket::disconnected, this, &NetworkWorker::onSocketDisconnected);

	// 发起连接
	m_socket->connectToHost(host, port);
}

void NetworkWorker::onSocketConnected()
{
	LogWidget::instance()->addLog("NetworkWorker: Connected to server", LogWidget::Info);
	emit connectedToServer();

	// 连接成功后发送 RequestRelay 消息
	sendRequestRelay();
}

// 发送 RequestRelay 消息
void NetworkWorker::sendRequestRelay()
{
	RendezvousMessage msg;
	RequestRelay* req = msg.mutable_request_relay();
	req->set_uuid(m_uuid.toUtf8().constData(), m_uuid.toUtf8().size());

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		LogWidget::instance()->addLog("Failed to serialize RequestRelay message", LogWidget::Error);
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(data);
		m_socket->flush();
		LogWidget::instance()->addLog(QString("Sent RequestRelay with uuid=%1").arg(m_uuid), LogWidget::Info);
	}
}

// 收到数据后拆包，然后发射 packetReady
void NetworkWorker::onSocketReadyRead()
{
	// 将收到的数据追加到缓冲区
	m_buffer.append(m_socket->readAll());

	// 协议： [4字节大端序包长] + [包数据]
	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		// 如果数据不足一个包，就 break
		if (m_buffer.size() < 4 + (int)packetSize) {
			break;
		}

		// 拆一帧数据
		QByteArray packetData = m_buffer.mid(4, packetSize);
		m_buffer.remove(0, 4 + packetSize);

		// 发射信号给解码线程
		emit packetReady(packetData);
	}
}

void NetworkWorker::onSocketError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	if (m_socket) {
		QString err = m_socket->errorString();
		LogWidget::instance()->addLog(QString("Socket Error: %1").arg(err), LogWidget::Error);
		emit networkError(err);
	}
}

void NetworkWorker::onSocketDisconnected()
{
	LogWidget::instance()->addLog("Socket disconnected", LogWidget::Warning);
	emit networkError("Disconnected from server");
}
