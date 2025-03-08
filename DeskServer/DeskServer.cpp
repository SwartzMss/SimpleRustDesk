#include "DeskServer.h"
#include <QtNetwork/QHostAddress>
#include "LogWidget.h"

DeskServer::DeskServer(QWidget* parent)
	: QWidget(parent), m_peerClient(nullptr)
{
	ui.setupUi(this);
	LogWidget::instance()->init(ui.widget_2);
	connect(ui.startButton_, &QPushButton::clicked, this, &DeskServer::onStartClicked);
	ui.startButton_->setText("Start");
}

DeskServer::~DeskServer()
{
	if (m_peerClient) {
		m_peerClient->stop();
		m_peerClient->deleteLater();
	}
	if (m_relayPeerClient) {
		m_relayPeerClient->stop();
		m_relayPeerClient->deleteLater();
	}
}

void DeskServer::onStartClicked()
{
	if (!m_peerClient) {
		QString ip = ui.iPLineEdit->text().trimmed();
		int port = ui.portLineEdit_->text().toInt();
		if (ip.isEmpty() || port <= 0) {
			LogWidget::instance()->addLog("IP or Port is invalid", LogWidget::Error);
			return;
		}

		QString relayIP = ui.iPLineEdit_3->text().trimmed();
		int relayPort = ui.portLineEdit_2->text().toInt();
		if (relayIP.isEmpty() || relayPort <= 0) {
			LogWidget::instance()->addLog("Relay IP or Relay Port is invalid", LogWidget::Error);
			return;
		}

		m_peerClient = new PeerClient(this);
		connect(m_peerClient, &PeerClient::registrationResult, this, &DeskServer::onRegistrationResult);
		connect(m_peerClient, &PeerClient::errorOccurred, this, &DeskServer::onClientError);
		m_peerClient->start(QHostAddress(ip), static_cast<quint16>(port));
		ui.iPLineEdit->setEnabled(false);
		ui.portLineEdit_->setEnabled(false);
		ui.startButton_->setText("Stop");

		m_relayPeerClient = new RelayPeerClient(this);
		// 当收到 heartbeat 回复时，更新 Relay 状态为 Online
		connect(m_relayPeerClient, &RelayPeerClient::heartbeatResponseReceived, this, [this]() {
			ui.label_8->setText("Online");
			});
		// 当出现错误时，记录错误并可将状态设置为 Offline
		connect(m_relayPeerClient, &RelayPeerClient::errorOccurred, this, [this](const QString& errorString) {
			ui.label_8->setText("Offline");
			LogWidget::instance()->addLog(errorString, LogWidget::Warning);
			});
		m_relayPeerClient->start(QHostAddress(relayIP), static_cast<quint16>(relayPort));
	}
	else {
		m_peerClient->stop();
		m_peerClient->deleteLater();
		m_peerClient = nullptr;
		if (m_relayPeerClient) {
			m_relayPeerClient->stop();
			m_relayPeerClient->deleteLater();
			m_relayPeerClient = nullptr;
		}
		ui.iPLineEdit->setEnabled(true);
		ui.portLineEdit_->setEnabled(true);
		ui.startButton_->setText("Start");
	}
}

void DeskServer::updateStatus(bool online)
{
	if (online) {
		ui.label_3->setText("Online");
	}
	else {
		ui.label_3->setText("Offline");
	}
}


void DeskServer::onRegistrationResult(int result)
{
	if (result == 0) {
		LogWidget::instance()->addLog(" Registration successful", LogWidget::Info);
		updateStatus(true);
	}
	else {
		LogWidget::instance()->addLog(QString(" Registration failed %1").arg(result),LogWidget::Warning);
		updateStatus(false);
	}
}

void DeskServer::onClientError(const QString& errorString)
{
	LogWidget::instance()->addLog(errorString, LogWidget::Warning);
	updateStatus(false);
}
