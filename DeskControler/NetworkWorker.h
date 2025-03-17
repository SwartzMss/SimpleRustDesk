#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QByteArray>
#include "MessageHandler.h"

class NetworkWorker : public QObject
{
	Q_OBJECT
public:
	explicit NetworkWorker(QObject* parent = nullptr);
	~NetworkWorker();

public slots:
	// �ڹ����߳�����ã����ӵ�ָ������������������
	void connectToServer(const QString& host, quint16 port, const QString& uuid);
	void cleanup();
	void sendMouseEventToServer(int x, int y, int mask);


signals:
	// �������һ֡ H264 ���ݺ󣬷����źŸ������߳�
	void packetReady(const QByteArray& packetData);
	// ��������Ͽ����źţ�����֪ͨ���߳�
	void networkError(const QString& error);
	void connectedToServer();

private slots:
	void onSocketConnected();
	void onSocketReadyRead();
	void onSocketError(QAbstractSocket::SocketError socketError);
	void onSocketDisconnected();

private:
	void sendRequestRelay();

private:
	QTcpSocket* m_socket = nullptr;
	QByteArray m_buffer;
	QString m_uuid;
	QString m_host;
	quint16 m_port;
	MessageHandler messageHandler;

};

#endif // NETWORKWORKER_H
