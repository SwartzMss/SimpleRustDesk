#pragma once

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include "MessageHandler.h"

class NetworkManager : public QObject
{
	Q_OBJECT
public:
	explicit NetworkManager(QObject* parent = nullptr);
	~NetworkManager();

	// ���� TCP ����
	bool connectToServer(const QString& ip, quint16 port);
	// ���� PunchHoleRequest ��Ϣ���ڲ����� MessageHandler��
	void sendPunchHoleRequest(const QString& uuid);

signals:
	// ֱ�ӽ��ڲ� MessageHandler ������ PunchHoleResponse ��������� UI ��
	void punchHoleResponseReceived(const QString& relayServer, int relayPort, int result);
	// �������Ϣ��������
	void networkError(const QString& error);
	// ���ӶϿ�
	void disconnected();

private slots:
	void onReadyRead();
	void onSocketDisconnected();

private:
	QTcpSocket* socket;
	MessageHandler messageHandler;  // �ڲ�������Ϣ�����߼�
};
