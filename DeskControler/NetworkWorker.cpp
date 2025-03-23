#include "NetworkWorker.h"
#include "LogWidget.h"
#include "rendezvous.pb.h"
#include <QUrl>
#include <QtNetwork/QHostInfo>
#include <QtEndian>
#include <QKeyEvent>

NetworkWorker::NetworkWorker(QObject* parent)
	: QObject(parent)
{
	connect(&messageHandler, &MessageHandler::InpuVideoFrameReceived,
		this, &NetworkWorker::packetReady);

	connect(&messageHandler, &MessageHandler::onClipboardMessageReceived,
		this, &NetworkWorker::onClipboardMessageReceived);
}

NetworkWorker::~NetworkWorker()
{
	cleanup();
}


void NetworkWorker::cleanup()
{
	if (m_socket) {
		m_socket->disconnect();
		if (m_socket->state() != QAbstractSocket::UnconnectedState) {
			m_socket->disconnectFromHost();
		}
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	m_buffer.clear();
}

void NetworkWorker::connectToServer(const QString& ip, quint16 port, const QString& uuid)
{

	// 解析输入，支持 URL 格式
	QUrl url = QUrl::fromUserInput(ip);
	// 如果 URL 解析成功，则提取 host，否则使用原始字符串
	QString host = url.host().isEmpty() ? ip : url.host();

	LogWidget::instance()->addLog(QString("Connecting to host: %1, port: %2").arg(host).arg(port), LogWidget::Info);

	QHostAddress resolvedAddress;
	if (!resolvedAddress.setAddress(host)) {
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
			QString error = QString("Failed to resolve host: %1").arg(host);
			LogWidget::instance()->addLog(error, LogWidget::Error);
			return;
		}
		// 遍历地址列表，筛选 IPv4 地址
		bool foundIPv4 = false;
		for (const QHostAddress& address : info.addresses()) {
			if (address.protocol() == QAbstractSocket::IPv4Protocol) {
				resolvedAddress = address;
				foundIPv4 = true;
				break;
			}
		}
		if (!foundIPv4) {
			LogWidget::instance()->addLog("No IPv4 address found for Relay IP: " + host, LogWidget::Error);
			return;
		}
		LogWidget::instance()->addLog(QString("Resolved host %1 to %2").arg(host, resolvedAddress.toString()), LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog(QString("Host address directly parsed: %1").arg(host), LogWidget::Info);
	}

	m_host = resolvedAddress.toString();
	m_port = port;
	m_uuid = uuid;

	if (m_socket) {
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	m_socket = new QTcpSocket(this);

	connect(m_socket, &QTcpSocket::connected, this, &NetworkWorker::onSocketConnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &NetworkWorker::onSocketReadyRead);
	connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkWorker::onSocketError);
	connect(m_socket, &QTcpSocket::disconnected, this, &NetworkWorker::onSocketDisconnected);

	m_socket->connectToHost(host, port);
}

void NetworkWorker::onSocketConnected()
{
	QString info = QString("Connected to server [%1:%2]").arg(m_socket->peerAddress().toString()).arg(m_socket->peerPort());
	LogWidget::instance()->addLog(info, LogWidget::Info);
	emit connectedToServer();

	// 连接成功后发送 RequestRelay 消息
	sendRequestRelay();
}

void NetworkWorker::sendRequestRelay()
{
	RendezvousMessage msg;
	RequestRelay* req = msg.mutable_request_relay();
	req->set_uuid(m_uuid.toUtf8().constData(), m_uuid.toUtf8().size());
	req->set_role(RequestRelay_DeskRole_DESK_CONTROL);

	std::string outStr;
	if (!msg.SerializeToString(&outStr)) {
		LogWidget::instance()->addLog("Failed to serialize RequestRelay message", LogWidget::Error);
		return;
	}
	QByteArray data(outStr.data(), static_cast<int>(outStr.size()));

	quint32 packetSize = static_cast<quint32>(data.size());
	quint32 bigEndianSize = qToBigEndian(packetSize);
	QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

	QByteArray fullData;
	fullData.append(header);
	fullData.append(data);

	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(fullData);
		m_socket->flush();
		LogWidget::instance()->addLog(QString("Sent RequestRelay to [%1:%2] with uuid=%3")
			.arg(m_socket->peerAddress().toString())
			.arg(m_socket->peerPort())
			.arg(m_uuid), LogWidget::Info);
	}
}


void NetworkWorker::onSocketReadyRead()
{
	m_buffer.append(m_socket->readAll());

	// 协议： [4字节大端序包长] + [包数据]
	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);

		if (m_buffer.size() < 4 + (int)packetSize) {
			break;
		}

		QByteArray packetData = m_buffer.mid(4, packetSize);
		m_buffer.remove(0, 4 + packetSize);

		messageHandler.processReceivedData(packetData);
	}
}


void NetworkWorker::sendMouseEventToServer(int x, int y, int mask)
{
	MouseEvent mouseEvent;
	mouseEvent.set_x(x);
	mouseEvent.set_y(y);
	mouseEvent.set_mask(mask);

	InputControlEvent inputEvent;
	*inputEvent.mutable_mouse_event() = mouseEvent;

	RendezvousMessage msg;
	*msg.mutable_inputcontrolevent() = inputEvent;

	std::string serialized;
	if (!msg.SerializeToString(&serialized)) {
		LogWidget::instance()->addLog("Failed to serialize MouseEvent message", LogWidget::Error);
		return;
	}

	QByteArray protobufData(serialized.data(), serialized.size());

	// 计算长度头（大端序）
	quint32 len = static_cast<quint32>(protobufData.size());
	quint32 len_be = qToBigEndian(len); // 大端序转换

	// 构造完整数据
	QByteArray sendData;
	sendData.append(reinterpret_cast<const char*>(&len_be), sizeof(len_be));
	sendData.append(protobufData);

	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(sendData);
		m_socket->flush();
	}
}

void NetworkWorker::sendKeyEventToServer(int key, bool pressed)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		return;  // 如果没有连接上 RelayServer，就不发送
	}

	// 组装 KeyboardEvent Protobuf 消息
	KeyboardEvent keyboardEvent;
	keyboardEvent.set_key(key);
	keyboardEvent.set_pressed(pressed);

	InputControlEvent inputEvent;
	*inputEvent.mutable_keyboard_event() = keyboardEvent;

	RendezvousMessage msg;
	*msg.mutable_inputcontrolevent() = inputEvent;

	std::string serialized;
	if (!msg.SerializeToString(&serialized)) {
		LogWidget::instance()->addLog("Failed to serialize MouseEvent message", LogWidget::Error);
		return;
	}

	QByteArray protobufData(serialized.data(), serialized.size());

	// 计算长度头（大端序）
	quint32 len = static_cast<quint32>(protobufData.size());
	quint32 len_be = qToBigEndian(len); // 大端序转换

	// 构造完整数据
	QByteArray sendData;
	sendData.append(reinterpret_cast<const char*>(&len_be), sizeof(len_be));
	sendData.append(protobufData);

	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(sendData);
		m_socket->flush();
	}
}

void NetworkWorker::sendClipboardEventToServer(const ClipboardEvent& clipboardEvent)
{
	if (m_socket->state() != QAbstractSocket::ConnectedState) {
		return;  // 如果没有连接上 RelayServer，就不发送
	}

	// 组装 ClipboardEvent 消息到 RendezvousMessage 中
	RendezvousMessage msg;
	*msg.mutable_clipboardevent() = clipboardEvent;

	std::string serialized;
	if (!msg.SerializeToString(&serialized)) {
		LogWidget::instance()->addLog("Failed to serialize ClipboardEvent message", LogWidget::Error);
		return;
	}

	QByteArray protobufData(serialized.data(), serialized.size());

	// 计算长度头（大端序）
	quint32 len = static_cast<quint32>(protobufData.size());
	quint32 len_be = qToBigEndian(len); // 大端序转换

	// 构造完整数据
	QByteArray sendData;
	sendData.append(reinterpret_cast<const char*>(&len_be), sizeof(len_be));
	sendData.append(protobufData);

	LogWidget::instance()->addLog("sendClipboardEventToServer", LogWidget::Error);
	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
		m_socket->write(sendData);
		m_socket->flush();
	}
}



void NetworkWorker::onSocketError(QAbstractSocket::SocketError socketError)
{
	Q_UNUSED(socketError);
	if (m_socket) {
		QString err = QString("Socket Error [%1:%2]: %3")
			.arg(m_host).arg(m_socket->peerPort()).arg(m_socket->errorString());
		LogWidget::instance()->addLog(err, LogWidget::Error);
		emit networkError(err);
	}
}

void NetworkWorker::onSocketDisconnected()
{
	QString info = QString("Socket disconnected from [%1:%2]").arg(m_socket->peerAddress().toString()).arg(m_socket->peerPort());
	LogWidget::instance()->addLog(info, LogWidget::Warning);
	emit networkError(info);
}

