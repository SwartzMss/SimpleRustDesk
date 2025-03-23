#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <QObject>
#include <QThread>
#include <QtNetwork/QHostAddress>
#include "RemoteInputSimulator.h"
#include "ScreenCaptureEncoder.h"
#include "RelaySocketWorker.h"
#include "RemoteClipboard.h"

class RelayManager : public QObject {
	Q_OBJECT
public:
	explicit RelayManager(QObject* parent = nullptr);
	~RelayManager();

	void start(const QHostAddress& relayAddress, quint16 relayPort, const QString& uuid);
	// 停止 TCP 连接及捕获/编码。
	void stop();

signals:
	void errorOccurred(const QString& errorString);
	void connected();
	void disconnected();

private slots:
	void onWorkerSocketConnected();
	void onWorkerSocketDisconnected();
	void onWorkerDataReceived(const QByteArray& data);
	void onWorkerSocketError(const QString& errMsg);
	void onEncodedPacketReady(const QByteArray& packet);
	void sendClipboardEvent(const ClipboardEvent& clipboardEvent);


private:
	void processReceivedData(const QByteArray& packetData);

private:
	RelaySocketWorker* m_socketWorker;
	QThread* m_socketThread;
	QHostAddress m_relayAddress;
	quint16 m_relayPort;
	QString m_uuid;
	QByteArray m_buffer;

	ScreenCaptureEncoder* m_encoder;
	RemoteInputSimulator* m_inputSimulator;
	QThread* m_encoderThread;
	RemoteClipboard* m_remoteClipboard;
};

#endif // RELAYMANAGER_H
