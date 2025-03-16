#pragma once

#include <QObject>
#include "rendezvous.pb.h"

class MessageHandler : public QObject {
	Q_OBJECT
public:
	explicit MessageHandler(QObject* parent = nullptr);
	~MessageHandler();

	// 生成 PunchHoleRequest 消息，uuid 转为 bytes
	QByteArray createPunchHoleRequestMessage(const QString& uuid);

	// 处理接收到的数据，解析后发出相应信号
	void processReceivedData(const QByteArray& data);

signals:
	// 当接收到 PunchHoleResponse 时发出信号，反馈 relay_server、relay_port 和结果（枚举值）
	void punchHoleResponseReceived(const QString& relayServer, int relayPort, int result);

	void InpuVideoFrameReceived(const QByteArray& packetData);
	// 消息解析错误
	void parseError(const QString& error);

};
