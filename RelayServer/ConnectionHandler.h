#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QtNetWork/QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <memory>
#include "rendezvous.pb.h"

class ConnectionHandler : public QObject
{
	Q_OBJECT
public:
	explicit ConnectionHandler( QObject* parent = nullptr);
	~ConnectionHandler();

	// 启动该连接处理器
	bool start(qintptr socketDescriptor);

	// 与对端连接配对
	void pairWith(std::shared_ptr<ConnectionHandler> peer);

	// 启动超时定时器（单位：毫秒）
	void startTimeout(int msec);

	// 断开当前连接及其对端连接
	void disconnectFromPeer();

	// 获取内部的 QTcpSocket 指针
	QTcpSocket* socket();

signals:
	// 当解析到包含 UUID 的中继请求时发射该信号
	void relayRequestReceived(const QString& uuid);

private slots:
	// 处理 socket 的 readyRead 信号
	void onReadyRead();
	// 超时处理槽
	void onTimeout();

private:
	QTcpSocket m_socket;
	QTimer m_timer;
	QByteArray m_buffer;
	// 配对的对端连接
	std::shared_ptr<ConnectionHandler> m_peer;

	// 解析完整消息
	void processData(const QByteArray& data);
};

#endif // CONNECTIONHANDLER_H
