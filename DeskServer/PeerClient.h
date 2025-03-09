#pragma once

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include "rendezvous.pb.h"

class PeerClient : public QObject {
	Q_OBJECT
public:
	explicit PeerClient(QObject* parent = nullptr);
	~PeerClient();

	// �������ӣ���������� IP �Ͷ˿�
	void start(const QHostAddress& address, quint16 port);
	// ֹͣ����
	void stop();
	// ���� relay ��Ϣ
	void setRelayInfo(const QString& ip, int port);

	void setRelayStatus(bool isOnline);

signals:
	// ע�����źţ����� RegisterPeerResponse::Result ö��ֵ
	void registrationResult(RegisterPeerResponse::Result result);
	// �����ź�
	void errorOccurred(const QString& errorString);

private slots:
	void onConnected();
	void onReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);
	void onDisconnected();
	void attemptReconnect();

private:
	void doConnect();

private:
	QTcpSocket* m_socket;
	QHostAddress m_serverAddress;
	quint16 m_serverPort;
	QTimer* m_reconnectTimer;
	bool m_isStopping;  // ����Ƿ�Ϊ����ֹͣ
	bool m_connected;
	bool m_isRelayOnline;
	QString m_relayIP;
	int m_relayPort;

};

