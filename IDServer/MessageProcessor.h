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

	// ������յ�����Ϣ��data ��ʽΪ JSON��
	void processMessage(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
signals:
	// �������ͻظ����źţ����ϲ�����ģ����÷��ͺ���
	void sendResponse(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
private:
	// ������Ϣ�����֧
	void processRegisterPeer(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processRegisterPk(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processPunchHoleRequest(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	void processRelayResponse(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	// ��������
	QString encodeAddress(const QHostAddress& addr, quint16 port);
	void sendRegisterPkResponse(const QString& result, QTcpSocket* tcpSocket, bool isUDP,
		const QHostAddress& sender, quint16 senderPort);
	PeerManager peerManager;
};

#endif // MESSAGEPROCESSOR_H
