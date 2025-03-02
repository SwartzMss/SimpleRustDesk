#include "RelayServer.h"
#include "LogWidget.h"
#include <QObject>

RelayServer::RelayServer(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

	LogWidget::instance()->init(ui.widget_3);

	connect(ui.startButton_, &QPushButton::clicked, this, &RelayServer::start);
	connect(ui.stopButton_, &QPushButton::clicked, this, &RelayServer::stop);
	ui.stopButton_->setEnabled(false);

	connect(&m_server, &QTcpServer::newConnection, this, &RelayServer::handleNewConnection);
}

RelayServer::~RelayServer()
{
}

void RelayServer::handleNewConnection()
{
	// �������еȴ�����
	while (m_server.hasPendingConnections()) {
		QTcpSocket* socket = m_server.nextPendingConnection();
		LogWidget::instance()->addLog(QString("received new connection %1").arg(socket->peerAddress().toString()));

		auto handler = std::make_shared<ConnectionHandler>();
		if (!handler->start(socket->socketDescriptor())) {
			LogWidget::instance()->addLog(
				QString("Failed to initialize ConnectionHandler for connection from %1").arg(socket->peerAddress().toString()),
				LogWidget::Error
			);
			socket->disconnectFromHost();
			continue;
		}

		// �� ConnectionHandler �յ��м�����ʱ������ relayRequestReceived �źŽ������
		connect(handler.get(), &ConnectionHandler::relayRequestReceived, this,
			[=](const QString& uuid) {
				tryPairing(uuid, handler);
			});
	}
}


void RelayServer::start()
{
	QString text = ui.lineEdit->text();
	bool isNumber = false;
	int port = text.toInt(&isNumber);
	if (isNumber)
	{

		if (!m_server.listen(QHostAddress::Any, port)) {
			LogWidget::instance()->addLog("Server start failed", LogWidget::Error);
			return;
		}
		LogWidget::instance()->addLog(QString("Server start succeed on port %1").arg(port), LogWidget::Info);
		ui.startButton_->setEnabled(false);
		ui.stopButton_->setEnabled(true);
		ui.lineEdit->setEnabled(false);
	}
	else
	{
		LogWidget::instance()->addLog("Port is invalid", LogWidget::Error);
	}
}

void RelayServer::stop()
{
	// �رշ�����
	m_server.close();

	// �Ͽ������ѽ���������
	for (auto& handler : mPeers) {
		handler->disconnectFromPeer();
	}

	// ��յȴ��б�
	mPeers.clear();

	ui.stopButton_->setEnabled(false);
	ui.startButton_->setEnabled(true);
	ui.lineEdit->setEnabled(true);
	LogWidget::instance()->addLog("Server stopped", LogWidget::Info);

}

void RelayServer::tryPairing(const QString& uuid, std::shared_ptr<ConnectionHandler> handler)
{
	// ������еȴ��е����ӣ������м̣��������ȴ��б�
	if (mPeers.contains(uuid)) {
		auto peer = mPeers.take(uuid);
		// ����˫������ת��
		handler->pairWith(peer);
		peer->pairWith(handler);
		// qDebug() << "�ɹ�ƥ�� UUID:" << uuid;
	}
	else {
		mPeers.insert(uuid, handler);
		// ����һ����ʱ��������ʱδ�����Ͽ�����
		handler->startTimeout(30000);  // 30�볬ʱ
		//qDebug() << "�ȴ�ƥ�� UUID:" << uuid;
	}
}