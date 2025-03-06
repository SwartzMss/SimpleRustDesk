#pragma once

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include "rendezvous.pb.h"

class PeerClient : public QObject {
	Q_OBJECT
public:
	explicit PeerClient(QObject* parent = nullptr);
	~PeerClient();

	// �������ӣ���������� IP �Ͷ˿�
	bool start(const QHostAddress& address, quint16 port);
	// ֹͣ����
	void stop();

signals:
	// ע�����źţ����� RegisterPeerResponse::Result ö��ֵ
	void registrationResult(RegisterPeerResponse::Result result);
	// �����ź�
	void errorOccurred(const QString& errorString);

private slots:
	void onConnected();
	void onReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);

private:
	QTcpSocket* m_socket;
	QHostAddress m_serverAddress;
	quint16 m_serverPort;
};

