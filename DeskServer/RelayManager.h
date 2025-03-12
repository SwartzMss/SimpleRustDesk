#ifndef RELAYMANAGER_H
#define RELAYMANAGER_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include "rendezvous.pb.h"
#include "ScreenCaptureEncoder.h"

class RelayManager : public QObject {
    Q_OBJECT
public:
    explicit RelayManager(QObject* parent = nullptr);
    ~RelayManager();

    void start(const QHostAddress& relayAddress, quint16 relayPort, const QString& uuid);
    // Stop TCP connection and stop capture/encoding.
    void stop();

signals:
    void errorOccurred(const QString& errorString);
    void connected();
    void disconnected();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    // Receive encoded data from ScreenCaptureEncoder and forward it.
    void onEncodedPacketReady(const QByteArray& packet);

private:
    QTcpSocket* m_socket;
    QHostAddress m_relayAddress;
    quint16 m_relayPort;
    QString m_uuid;
    QString m_punchHoleId;

    // Screen capture and encoder instance managed internally.
    ScreenCaptureEncoder* m_encoder;
};

#endif // RELAYMANAGER_H
