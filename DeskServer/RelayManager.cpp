#include "RelayManager.h"
#include "LogWidget.h"

RelayManager::RelayManager(QObject* parent)
    : QObject(parent),
      m_socket(nullptr),
      m_relayPort(0),
      m_encoder(nullptr)
{
	m_inputSimulator = new RemoteInputSimulator(this);
}

RelayManager::~RelayManager() {
    stop();
}

void RelayManager::start(const QHostAddress& relayAddress, quint16 relayPort, const QString& uuid)
{
    m_relayAddress = relayAddress;
    m_relayPort = relayPort;
    m_uuid = uuid;

    m_socket = new QTcpSocket(this);
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(m_socket, &QTcpSocket::connected, this, &RelayManager::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RelayManager::onSocketDisconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &RelayManager::onReadyRead);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    LogWidget::instance()->addLog(
        QString("RelayManager: Attempting to connect to %1:%2")
            .arg(m_relayAddress.toString()).arg(m_relayPort),
        LogWidget::Info);
    m_socket->connectToHost(m_relayAddress, m_relayPort);


	m_encoderThread = new QThread(this);
	m_encoder = new ScreenCaptureEncoder(); // ���ڲ�ָ��������
	m_encoder->moveToThread(m_encoderThread);

	// ���߳�����ʱ�����ñ������� startCapture ����
	connect(m_encoderThread, &QThread::started, m_encoder, &ScreenCaptureEncoder::startCapture);
	// ���ձ�������ݵ��źţ�ע��������Ȼ�� RelayManager �Ĳ۴����ۻ������߳���ִ�У�
	connect(m_encoder, &ScreenCaptureEncoder::encodedPacketReady, this, &RelayManager::onEncodedPacketReady);
	// �߳̽���ʱ���������
	connect(m_encoderThread, &QThread::finished, m_encoder, &QObject::deleteLater);

	m_encoderThread->start();
}

void RelayManager::stop()
{
    // Disconnect and cleanup TCP socket.
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
	if (m_encoder) {
		// ʹ�� Qt::BlockingQueuedConnection ȷ�� stopCapture �ڱ����߳���ִ�����
		QMetaObject::invokeMethod(m_encoder, "stopCapture", Qt::BlockingQueuedConnection);
	}
	if (m_encoderThread) {
		m_encoderThread->quit();
		m_encoderThread->wait();
		m_encoderThread->deleteLater();
		m_encoderThread = nullptr;
	}
}

void RelayManager::onReadyRead()
{
	m_buffer.append(m_socket->readAll());

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

void RelayManager::processReceivedData(const QByteArray& packetData)
{
	RendezvousMessage msg;

	if (!msg.ParseFromArray(packetData.constData(), packetData.size())) {
		LogWidget::instance()->addLog("Failed to parse RendezvousMessage message from RelayManager", LogWidget::Warning);
		return;
	}

	if (msg.has_inputcontrolevent()) {
		const InputControlEvent& event = msg.inputcontrolevent();
		if (event.has_mouse_event()) {
			const MouseEvent& mouseEvent = event.mouse_event();
			int x = mouseEvent.x();
			int y = mouseEvent.y();
			int mask = mouseEvent.mask();

			//LogWidget::instance()->addLog(
			//	QString("MessageHandler: received MouseEvent x=%1 y=%2 mask=%3").arg(x).arg(y).arg(mask),
			//	LogWidget::Info
			//);
			m_inputSimulator->handleMouseEvent(x, y, mask);
		}
		else if (event.has_keyboard_event()) {
			const KeyboardEvent& keyboardEvent = event.keyboard_event();
			int key = keyboardEvent.key();  // ��ȡ��ֵ
			bool pressed = keyboardEvent.pressed();  // ��ȡ����״̬������ / �ͷţ�

			//LogWidget::instance()->addLog(
			//	QString("RelayManager: received KeyboardEvent key=%1 pressed=%2")
			//	.arg(key).arg(pressed ? "true" : "false"),
			//	LogWidget::Info
			//);

			m_inputSimulator->handleKeyboardEvent(key, pressed);
		}
	}
}


void RelayManager::onSocketConnected()
{
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

		m_socket->write(fullData);
		m_socket->flush();
		LogWidget::instance()->addLog(QString("RelayManager: RequestRelay message sent %1").arg(m_uuid), LogWidget::Info);
	}
	else {
		emit errorOccurred("RelayManager: Failed to serialize RequestRelay message");
	}
}

void RelayManager::onSocketDisconnected()
{
    LogWidget::instance()->addLog("RelayManager: TCP connection to relay server disconnected", LogWidget::Warning);
    emit disconnected();
    if (m_encoder) {
        m_encoder->stopCapture();
        LogWidget::instance()->addLog("RelayManager: Stopped screen encoding due to connection loss", LogWidget::Warning);
    }
}

void RelayManager::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errMsg = m_socket ? m_socket->errorString() : "Unknown error";
    LogWidget::instance()->addLog("RelayManager: " + errMsg, LogWidget::Error);
    emit errorOccurred(errMsg);
}

void RelayManager::onEncodedPacketReady(const QByteArray& packet)
{
	if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
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

		m_socket->write(fullData);
		m_socket->flush();
	}
	else {
		LogWidget::instance()->addLog(
			"RelayManager: Cannot forward packet, TCP connection not established",
			LogWidget::Warning
		);
	}
}

