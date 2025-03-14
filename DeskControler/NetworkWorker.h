#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QByteArray>

class NetworkWorker : public QObject
{
	Q_OBJECT
public:
	explicit NetworkWorker(QObject* parent = nullptr);
	~NetworkWorker();

public slots:
	// �ڹ����߳�����ã����ӵ�ָ������������������
	void connectToServer(const QString& host, quint16 port, const QString& uuid);

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
	QTcpSocket* m_socket = nullptr;
	QByteArray m_buffer;
	QString m_uuid;
	QString m_host;
	quint16 m_port;
	void sendRequestRelay();
};

#endif // NETWORKWORKER_H
