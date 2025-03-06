#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include "PeerManager.h"
#include "rendezvous.pb.h"

class MessageProcessor : public QObject {
	Q_OBJECT
public:
	explicit MessageProcessor(QObject* parent = nullptr);

	// ������յ�����Ϣ��data ��ʽΪ JSON��
	void processMessage(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
signals:
	// �������ͻظ����źţ����ϲ�����ģ����÷��ͺ���
	void sendResponse(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);
private:
	// ������Ϣ�����֧
	void processRegisterPeer(const RegisterPeer& req, const QHostAddress& sender, quint16 senderPort);

	QByteArray createRegisterPeerResponse(RegisterPeerResponse::Result result);
	PeerManager peerManager;
};

#endif // MESSAGEPROCESSOR_H
