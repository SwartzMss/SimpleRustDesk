#include "RelayManager.h"
#include "LogWidget.h"
#include <QMetaObject>
#include <QtNetwork/QHostAddress>
#include <QByteArray>
#include <QThread>
#include "rendezvous.pb.h"

RelayManager::RelayManager(QObject* parent)
	: QObject(parent),
	m_socketWorker(nullptr),
	m_socketThread(nullptr),
	m_relayPort(0),
	m_encoder(nullptr)
{
	m_inputSimulator = new RemoteInputSimulator(nullptr);
	m_remoteClipboard = new RemoteClipboard(nullptr);
}

RelayManager::~RelayManager() {
	stop();
}

void RelayManager::start(const QHostAddress& relayAddress, quint16 relayPort, const QString& uuid) {
	m_relayAddress = relayAddress;
	m_relayPort = relayPort;
	m_uuid = uuid;

	// 创建后台线程和网络工作者
	m_socketThread = new QThread(this);
	m_socketWorker = new RelaySocketWorker();
	m_socketWorker->moveToThread(m_socketThread);
	connect(m_socketWorker, &RelaySocketWorker::socketConnected, this, &RelayManager::onWorkerSocketConnected);
	connect(m_socketWorker, &RelaySocketWorker::socketDisconnected, this, &RelayManager::onWorkerSocketDisconnected);
	connect(m_socketWorker, &RelaySocketWorker::dataReceived, this, &RelayManager::onWorkerDataReceived, Qt::DirectConnection);
	connect(m_socketWorker, &RelaySocketWorker::socketErrorOccurred, this, &RelayManager::onWorkerSocketError);
	m_socketThread->start();
	QMetaObject::invokeMethod(m_socketWorker, "connectToHost", Qt::QueuedConnection,
		Q_ARG(QHostAddress, m_relayAddress),
		Q_ARG(quint16, m_relayPort));
	LogWidget::instance()->addLog(
		QString("RelayManager: Attempting to connect to %1:%2")
		.arg(m_relayAddress.toString()).arg(m_relayPort),
		LogWidget::Info);

	m_encoderThread = new QThread(this);
	m_encoder = new ScreenCaptureEncoder();
	m_encoder->moveToThread(m_encoderThread);
	connect(m_encoderThread, &QThread::started, m_encoder, &ScreenCaptureEncoder::startCapture);
	connect(m_encoder, &ScreenCaptureEncoder::encodedPacketReady, this, &RelayManager::onEncodedPacketReady);
	connect(m_encoderThread, &QThread::finished, m_encoder, &QObject::deleteLater);
	connect(m_remoteClipboard, &RemoteClipboard::ctrlCPressed, this, &RelayManager::sendClipboardEvent);
	m_encoderThread->start();
	m_remoteClipboard->start();
}

void RelayManager::stop() {
	if (m_socketWorker) {
		QMetaObject::invokeMethod(m_socketWorker, "disconnectSocket", Qt::BlockingQueuedConnection);
	}
	if (m_socketThread) {
		m_socketThread->quit();
		m_socketThread->wait();
		m_socketThread->deleteLater();
		m_socketThread = nullptr;
		m_socketWorker = nullptr;
	}
	if (m_encoder) {
		QMetaObject::invokeMethod(m_encoder, "stopCapture", Qt::BlockingQueuedConnection);
	}
	if (m_encoderThread) {
		m_encoderThread->quit();
		m_encoderThread->wait();
		m_encoderThread->deleteLater();
		m_encoderThread = nullptr;
	}
	if (m_remoteClipboard) {
		m_remoteClipboard->stop();
		m_remoteClipboard->deleteLater();
		m_remoteClipboard = nullptr;
	}
}

void RelayManager::onWorkerSocketConnected() {
	LogWidget::instance()->addLog("RelayManager: Connected to relay server via TCP", LogWidget::Info);
	emit connected();

	RequestRelay req;
	req.set_uuid(m_uuid.toStdString());
	req.set_role(RequestRelay_DeskRole_DESK_SERVER);
	RendezvousMessage relayMsg;
	*relayMsg.mutable_request_relay() = req;
	std::string reqStr;
	if (relayMsg.SerializeToString(&reqStr)) {
		QByteArray data(reqStr.data(), static_cast<int>(reqStr.size()));
		quint32 packetSize = static_cast<quint32>(data.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));
		QByteArray fullData;
		fullData.append(header);
		fullData.append(data);
		QMetaObject::invokeMethod(m_socketWorker, "sendData", Qt::QueuedConnection,
			Q_ARG(QByteArray, fullData));
		LogWidget::instance()->addLog(QString("RelayManager: RequestRelay message sent %1").arg(m_uuid), LogWidget::Info);
	}
	else {
		emit errorOccurred("RelayManager: Failed to serialize RequestRelay message");
	}
}

void RelayManager::onWorkerSocketDisconnected() {
	LogWidget::instance()->addLog("RelayManager: TCP connection to relay server disconnected", LogWidget::Warning);
	emit disconnected();
	if (m_encoder) {
		m_encoder->stopCapture();
		LogWidget::instance()->addLog("RelayManager: Stopped screen encoding due to connection loss", LogWidget::Warning);
	}
}

void RelayManager::onWorkerSocketError(const QString& errMsg) {
	LogWidget::instance()->addLog("RelayManager: " + errMsg, LogWidget::Error);
	emit errorOccurred(errMsg);
}

void RelayManager::onWorkerDataReceived(const QByteArray& data) {
	m_buffer.append(data);
	while (m_buffer.size() >= 4) {
		quint32 packetSize;
		memcpy(&packetSize, m_buffer.constData(), 4);
		packetSize = qFromBigEndian(packetSize);
		if (m_buffer.size() < 4 + static_cast<int>(packetSize))
			break;
		QByteArray packetData = m_buffer.mid(4, packetSize);
		m_buffer.remove(0, 4 + packetSize);
		processReceivedData(packetData);
	}
}

void RelayManager::processReceivedData(const QByteArray& packetData) {
	RendezvousMessage msg;
	if (!msg.ParseFromArray(packetData.constData(), packetData.size())) {
		LogWidget::instance()->addLog("Failed to parse RendezvousMessage message from RelayManager", LogWidget::Warning);
		return;
	}
	LogWidget::instance()->addLog("processReceivedData", LogWidget::Warning);
	if (msg.has_inputcontrolevent()) {
		const InputControlEvent& event = msg.inputcontrolevent();
		if (event.has_mouse_event()) {
			const MouseEvent& mouseEvent = event.mouse_event();
			int x = mouseEvent.x();
			int y = mouseEvent.y();
			int mask = mouseEvent.mask();
			QMetaObject::invokeMethod(m_inputSimulator, "handleMouseEvent", Qt::QueuedConnection,
				Q_ARG(int, x), Q_ARG(int, y), Q_ARG(int, mask));
		}
		else if (event.has_keyboard_event()) {
			const KeyboardEvent& keyboardEvent = event.keyboard_event();
			int key = keyboardEvent.key();
			bool pressed = keyboardEvent.pressed();
			QMetaObject::invokeMethod(m_inputSimulator, "handleKeyboardEvent", Qt::QueuedConnection,
				Q_ARG(int, key), Q_ARG(bool, pressed));
		}
	}
	else if (msg.has_clipboardevent()) {

		const ClipboardEvent& clipboardEvent = msg.clipboardevent();
		QMetaObject::invokeMethod(m_remoteClipboard, "onClipboardMessageReceived", Qt::QueuedConnection,
			Q_ARG(ClipboardEvent, clipboardEvent));
	}
	else {
		LogWidget::instance()->addLog("Received unknown message type in RendezvousMessage", LogWidget::Warning);
	}
}

void RelayManager::onEncodedPacketReady(const QByteArray& packet) {
	if (m_socketWorker) {
		InpuVideoFrame videoFrame;
		videoFrame.set_data(packet.data(), packet.size());
		RendezvousMessage msg;
		*msg.mutable_inpuvideoframe() = videoFrame;
		std::string outStr;
		if (!msg.SerializeToString(&outStr)) {
			LogWidget::instance()->addLog("RelayManager: Failed to serialize InpuVideoFrame message", LogWidget::Error);
			return;
		}
		QByteArray data(outStr.data(), static_cast<int>(outStr.size()));
		quint32 packetSize = static_cast<quint32>(data.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));
		QByteArray fullData;
		fullData.append(header);
		fullData.append(data);
		QMetaObject::invokeMethod(m_socketWorker, "sendData", Qt::QueuedConnection,
			Q_ARG(QByteArray, fullData));
	}
	else {
		LogWidget::instance()->addLog("RelayManager: Cannot forward packet, TCP connection not established", LogWidget::Warning);
	}
}

void RelayManager::sendClipboardEvent(const ClipboardEvent& clipboardEvent)
{
	// 组装 ClipboardEvent 消息到 RendezvousMessage 中
	RendezvousMessage msg;
	*msg.mutable_clipboardevent() = clipboardEvent;

	std::string serialized;
	if (!msg.SerializeToString(&serialized)) {
		LogWidget::instance()->addLog("Failed to serialize ClipboardEvent message", LogWidget::Error);
		return;
	}

	QByteArray data(serialized.data(), static_cast<int>(serialized.size()));
	quint32 packetSize = static_cast<quint32>(data.size());
	quint32 bigEndianSize = qToBigEndian(packetSize);
	QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));
	QByteArray fullData;
	fullData.append(header);
	fullData.append(data);
	QMetaObject::invokeMethod(m_socketWorker, "sendData", Qt::QueuedConnection,
		Q_ARG(QByteArray, fullData));
}
