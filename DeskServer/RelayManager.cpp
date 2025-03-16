#include "RelayManager.h"
#include "LogWidget.h"

RelayManager::RelayManager(QObject* parent)
    : QObject(parent),
      m_socket(nullptr),
      m_relayPort(0),
      m_encoder(nullptr)
{
}

RelayManager::~RelayManager() {
    stop();
}

void RelayManager::start(const QHostAddress& relayAddress, quint16 relayPort, const QString& uuid)
{
    m_relayAddress = relayAddress;
    m_relayPort = relayPort;
    m_uuid = uuid;

    // Create TCP socket and establish connection.
    m_socket = new QTcpSocket(this);
    m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(m_socket, &QTcpSocket::connected, this, &RelayManager::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RelayManager::onSocketDisconnected);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketError(QAbstractSocket::SocketError)));

    LogWidget::instance()->addLog(
        QString("RelayManager: Attempting to connect to %1:%2")
            .arg(m_relayAddress.toString()).arg(m_relayPort),
        LogWidget::Info);
    m_socket->connectToHost(m_relayAddress, m_relayPort);

    // Create and start the screen capture and encoding.
    if (!m_encoder) {
        m_encoder = new ScreenCaptureEncoder(this);
        connect(m_encoder, &ScreenCaptureEncoder::encodedPacketReady, this, &RelayManager::onEncodedPacketReady);
    }
    m_encoder->startCapture();
}

void RelayManager::stop()
{
    // Disconnect and cleanup TCP socket.
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    // Stop screen capture and encoding.
    if (m_encoder) {
        m_encoder->stopCapture();
        m_encoder->deleteLater();
        m_encoder = nullptr;
    }
}

void RelayManager::onSocketConnected()
{
    LogWidget::instance()->addLog("RelayManager: Connected to relay server via TCP", LogWidget::Info);
    emit connected();

    // Construct and send RequestRelay message.
    RequestRelay req;
    req.set_uuid(m_uuid.toStdString());
    req.set_role(RequestRelay_DeskRole_DESK_SERVER);
    RendezvousMessage relayMsg;
    *relayMsg.mutable_request_relay() = req;
    std::string reqStr;
    if (relayMsg.SerializeToString(&reqStr)) {
        m_socket->write(reqStr.data(), reqStr.size());
        m_socket->flush();
        LogWidget::instance()->addLog(QString("RelayManager: RequestRelay message sent %1").arg(m_uuid), LogWidget::Info);
    } else {
        emit errorOccurred("RelayManager: Failed to serialize RequestRelay message");
    }
}

void RelayManager::onSocketDisconnected()
{
    LogWidget::instance()->addLog("RelayManager: TCP connection to relay server disconnected", LogWidget::Warning);
    emit disconnected();
    // Stop screen capture and encoding on disconnection.
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
		// 先构造 4 字节网络序（大端序）的帧长度
		quint32 packetSize = static_cast<quint32>(packet.size());
		quint32 bigEndianSize = qToBigEndian(packetSize);

		// 将这4个字节放到一个 QByteArray 里
		QByteArray header(reinterpret_cast<const char*>(&bigEndianSize), sizeof(bigEndianSize));

		// 组合： [长度头][帧数据]
		QByteArray fullData;
		fullData.append(header);
		fullData.append(packet);

		// 发送
		m_socket->write(fullData);
		m_socket->flush();

		//LogWidget::instance()->addLog(
		//	QString("RelayManager: Forwarded encoded packet (size %1 + 4-byte header)").arg(packet.size()),
		//	LogWidget::Info
		//);
	}
	else {
		LogWidget::instance()->addLog(
			"RelayManager: Cannot forward packet, TCP connection not established",
			LogWidget::Warning
		);
	}
}
