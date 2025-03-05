#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include "PeerManager.h"

class MessageProcessor : public QObject {
	Q_OBJECT
public:
	explicit MessageProcessor(QObject* parent = nullptr);

	// 处理接收到的消息（data 格式为 JSON）
	void processMessage(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
signals:
	// 发出发送回复的信号，由上层网络模块调用发送函数
	void sendResponse(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
private:
	// 各种消息处理分支
	void processRegisterPeer(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processRegisterPk(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processPunchHoleRequest(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processRelayResponse(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	// 辅助函数
	QString encodeAddress(const QHostAddress& addr, quint16 port);
	void sendRegisterPkResponse(const QString& result, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	PeerManager peerManager;
};

#endif // MESSAGEPROCESSOR_H
