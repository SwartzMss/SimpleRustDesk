#pragma once

#include <QObject>
#include "rendezvous.pb.h"

class MessageHandler : public QObject {
	Q_OBJECT
public:
	explicit MessageHandler(QObject* parent = nullptr);
	~MessageHandler();

	// ���� PunchHoleRequest ��Ϣ��uuid תΪ bytes
	QByteArray createPunchHoleRequestMessage(const QString& uuid);

	// ������յ������ݣ������󷢳���Ӧ�ź�
	void processReceivedData(const QByteArray& data);

signals:
	// �����յ� PunchHoleResponse ʱ�����źţ����� relay_server��relay_port �ͽ����ö��ֵ��
	void punchHoleResponseReceived(const QString& relayServer, int relayPort, int result);

	void InpuVideoFrameReceived(const QByteArray& packetData);
	// ��Ϣ��������
	void parseError(const QString& error);

};
