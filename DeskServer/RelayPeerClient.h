#pragma once

#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include "rendezvous.pb.h"

class RelayPeerClient : public QObject {
	Q_OBJECT
public:
	explicit RelayPeerClient(QObject* parent = nullptr);
	~RelayPeerClient();

	// ���� RelayPeerClient������ Relay �� IP �� Port
	void start(const QHostAddress& relayAddress, quint16 relayPort);
	// ֹͣ
	void stop();

signals:
	// �յ� Heartbeat �ظ�ʱ�����ź�
	void heartbeatResponseReceived();
	// �����ź�
	void errorOccurred(const QString& errorString);

private slots:
	// ��ʱ���� Heartbeat ��Ϣ
	void sendHeartbeat();
	// �����յ�������
	void onReadyRead();

private:
	QUdpSocket* m_udpSocket;
	QHostAddress m_relayAddress;
	quint16 m_relayPort;
	QTimer* m_heartbeatTimer;
};
