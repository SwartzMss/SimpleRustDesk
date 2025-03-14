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
	// 在工作线程里调用，连接到指定服务器并发送请求
	void connectToServer(const QString& host, quint16 port, const QString& uuid);

signals:
	// 当拆包出一帧 H264 数据后，发出信号给解码线程
	void packetReady(const QByteArray& packetData);
	// 网络出错、断开等信号，可以通知主线程
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
