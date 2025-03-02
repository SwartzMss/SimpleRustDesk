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
	// 处理所有等待连接
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

		// 当 ConnectionHandler 收到中继请求时，触发 relayRequestReceived 信号进行配对
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
	// 关闭服务器
	m_server.close();

	// 断开所有已建立的连接
	for (auto& handler : mPeers) {
		handler->disconnectFromPeer();
	}

	// 清空等待列表
	mPeers.clear();

	ui.stopButton_->setEnabled(false);
	ui.startButton_->setEnabled(true);
	ui.lineEdit->setEnabled(true);
	LogWidget::instance()->addLog("Server stopped", LogWidget::Info);

}

void RelayServer::tryPairing(const QString& uuid, std::shared_ptr<ConnectionHandler> handler)
{
	// 如果已有等待中的连接，则建立中继，否则加入等待列表
	if (mPeers.contains(uuid)) {
		auto peer = mPeers.take(uuid);
		// 建立双向数据转发
		handler->pairWith(peer);
		peer->pairWith(handler);
		// qDebug() << "成功匹配 UUID:" << uuid;
	}
	else {
		mPeers.insert(uuid, handler);
		// 启动一个定时器，若超时未配对则断开连接
		handler->startTimeout(30000);  // 30秒超时
		//qDebug() << "等待匹配 UUID:" << uuid;
	}
}