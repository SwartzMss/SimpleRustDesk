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

	// ���������Ӵ�����
	bool start(qintptr socketDescriptor);

	// ��Զ��������
	void pairWith(std::shared_ptr<ConnectionHandler> peer);

	// ������ʱ��ʱ������λ�����룩
	void startTimeout(int msec);

	// �Ͽ���ǰ���Ӽ���Զ�����
	void disconnectFromPeer();

	// ��ȡ�ڲ��� QTcpSocket ָ��
	QTcpSocket* socket();

signals:
	// ������������ UUID ���м�����ʱ������ź�
	void relayRequestReceived(const QString& uuid);

private slots:
	// ���� socket �� readyRead �ź�
	void onReadyRead();
	// ��ʱ�����
	void onTimeout();

private:
	QTcpSocket m_socket;
	QTimer m_timer;
	QByteArray m_buffer;
	// ��ԵĶԶ�����
	std::shared_ptr<ConnectionHandler> m_peer;

	// ����������Ϣ
	void processData(const QByteArray& data);
};

#endif // CONNECTIONHANDLER_H
