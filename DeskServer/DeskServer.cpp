#include "DeskServer.h"
#include <QtNetwork/QHostAddress>
#include "LogWidget.h"

DeskServer::DeskServer(QWidget* parent)
	: QWidget(parent), m_peerClient(nullptr)
{
	ui.setupUi(this);
	LogWidget::instance()->init(ui.widget_2);
	connect(ui.startButton_, &QPushButton::clicked, this, &DeskServer::onStartClicked);
	connect(ui.stopButton_, &QPushButton::clicked, this, &DeskServer::onStopClicked);
	ui.stopButton_->setEnabled(false);
}

DeskServer::~DeskServer()
{
	if (m_peerClient) {
		m_peerClient->stop();
		m_peerClient->deleteLater();
	}
}

void DeskServer::onStartClicked()
{
	QString ip = ui.iPLineEdit->text().trimmed();
	int port = ui.portLineEdit_->text().toInt();
	if (ip.isEmpty() || port <= 0) {
		LogWidget::instance()->addLog(" Ip or Port is invalid", LogWidget::Error);
		return;
	}

	m_peerClient = new PeerClient(this);
	connect(m_peerClient, &PeerClient::registrationResult, this, &DeskServer::onRegistrationResult);
	connect(m_peerClient, &PeerClient::errorOccurred, this, &DeskServer::onClientError);
	if (!m_peerClient->start(QHostAddress(ip), static_cast<quint16>(port)))
	{
		return;
	}
	;

	ui.portLineEdit_->setEnabled(false);
	ui.iPLineEdit->setEnabled(false);
	ui.startButton_->setEnabled(false);
	ui.stopButton_->setEnabled(true);
}

void DeskServer::onStopClicked()
{
	if (m_peerClient) {
		m_peerClient->stop();
		m_peerClient->deleteLater();
		m_peerClient = nullptr;
	}
	ui.portLineEdit_->setEnabled(true);
	ui.iPLineEdit->setEnabled(true);
	ui.startButton_->setEnabled(true);
	ui.stopButton_->setEnabled(false);
}

void DeskServer::onRegistrationResult(int result)
{
	if (result == 0) {
		LogWidget::instance()->addLog(" Registration successful", LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog(QString(" Registration failed %1").arg(result),LogWidget::Warning);
	}
}

void DeskServer::onClientError(const QString& errorString)
{
	LogWidget::instance()->addLog(QString(" PeerClient error: %1").arg(errorString), LogWidget::Warning);
}
