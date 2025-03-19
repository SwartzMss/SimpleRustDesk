#ifndef RELAYSOCKETWORKER_H
#define RELAYSOCKETWORKER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

class RelaySocketWorker : public QObject {
	Q_OBJECT
public:
	explicit RelaySocketWorker(QObject* parent = nullptr);
	~RelaySocketWorker();

public slots:
	void connectToHost(const QHostAddress& address, quint16 port);
	void sendData(const QByteArray& data);
	void disconnectSocket();

private slots:
	void onReadyRead();
	void onSocketError(QAbstractSocket::SocketError error);

signals:
	void socketConnected();
	void socketDisconnected();
	void dataReceived(const QByteArray& data);
	void socketErrorOccurred(const QString& error);

private:
	QTcpSocket* m_socket;
};

#endif // RELAYSOCKETWORKER_H
