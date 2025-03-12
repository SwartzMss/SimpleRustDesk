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
	QString m_uuid;   // 保存要发送的 uuid (比如 RequestRelay 里要用)

	void sendRequestRelay();
};

#endif // NETWORKWORKER_H
