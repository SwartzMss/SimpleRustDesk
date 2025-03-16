#include "DeskServer.h"
#include <QtNetwork/QHostAddress>
#include <QUrl>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QHostInfo>
#include "LogWidget.h"

DeskServer::DeskServer(QWidget* parent)
	: QWidget(parent), m_peerClient(nullptr)
{
	ui.setupUi(this);
	loadConfig();
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


void DeskServer::loadConfig()
{
	QFile file("DeskServer.json");
	QJsonObject config;
	bool valid = false;

	if (file.exists()) {
		if (file.open(QIODevice::ReadOnly)) {
			QByteArray data = file.readAll();
			file.close();
			QJsonDocument doc = QJsonDocument::fromJson(data);
			// 如果 JSON 格式正确且是对象，就使用文件中的配置
			if (!doc.isNull() && doc.isObject()) {
				config = doc.object();
				valid = true;
			}
		}
	}

	// 如果文件不存在或格式不正确，则采用默认配置
	if (!valid) {
		config["server"] = QJsonObject{
			{"ip", "127.0.0.1"},
			{"port", 21116}
		};
		config["relay"] = QJsonObject{
			{"ip", "127.0.0.1"},
			{"port", 21117}
		};
		// 默认情况下生成一个新的 uuid
		config["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

		// 写入默认配置到文件
		if (file.open(QIODevice::WriteOnly)) {
			QJsonDocument doc(config);
			file.write(doc.toJson());
			file.close();
		}
		else {
			qWarning() << "Could not create or write to configuration file.";
		}
	}

	// 从配置中读取值
	QJsonObject serverObj = config["server"].toObject();
	QJsonObject relayObj = config["relay"].toObject();
	m_uuidStr = config["uuid"].toString() == "" ? QUuid::createUuid().toString(QUuid::WithoutBraces): config["uuid"].toString();

	// 设置 UI 输入框的默认值
	ui.iPLineEdit->setText(serverObj["ip"].toString("127.0.0.1"));
	ui.portLineEdit_->setText(QString::number(serverObj["port"].toInt(21116)));
	ui.iPLineEdit_3->setText(relayObj["ip"].toString("127.0.0.1"));
	ui.portLineEdit_2->setText(QString::number(relayObj["port"].toInt(21117)));
}


// 保存当前配置到文件
void DeskServer::saveConfig()
{
	QJsonObject config;
	QJsonObject serverObj;
	serverObj["ip"] = ui.iPLineEdit->text().trimmed();
	serverObj["port"] = ui.portLineEdit_->text().toInt();
	QJsonObject relayObj;
	relayObj["ip"] = ui.iPLineEdit_3->text().trimmed();
	relayObj["port"] = ui.portLineEdit_2->text().toInt();

	config["server"] = serverObj;
	config["relay"] = relayObj;

	config["uuid"] = m_uuidStr;

	QJsonDocument doc(config);
	QFile file("DeskServer.json");
	if (file.open(QIODevice::WriteOnly)) {
		file.write(doc.toJson());
		file.close();
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

		// 尝试解析用户输入，支持 URL 或域名格式
		QUrl url = QUrl::fromUserInput(ip);
		// 如果解析后的 host 不为空，说明输入了 URL 格式，取出 host，否则直接使用输入
		QString host = url.host().isEmpty() ? ip : url.host();
		QHostAddress resolvedAddress;
		// 先尝试直接转换成 IP 地址
		if (!resolvedAddress.setAddress(host)) {
			// 如果转换失败，则进行 DNS 解析
			QHostInfo info = QHostInfo::fromName(host);
			if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
				LogWidget::instance()->addLog("Failed to resolve IP: " + host, LogWidget::Error);
				return;
			}
			// 遍历地址列表，筛选 IPv4 地址
			bool foundIPv4 = false;
			for (const QHostAddress& address : info.addresses()) {
				if (address.protocol() == QAbstractSocket::IPv4Protocol) {
					resolvedAddress = address;
					foundIPv4 = true;
					break;
				}
			}
			if (!foundIPv4) {
				LogWidget::instance()->addLog("No IPv4 address found for Relay IP: " + host, LogWidget::Error);
				return;
			}
		}

		QString relayIP = ui.iPLineEdit_3->text().trimmed();
		int relayPort = ui.portLineEdit_2->text().toInt();
		if (relayIP.isEmpty() || relayPort <= 0) {
			LogWidget::instance()->addLog("Relay IP or Relay Port is invalid", LogWidget::Error);
			return;
		}

		// 同样处理 Relay IP
		QUrl relayUrl = QUrl::fromUserInput(relayIP);
		QString relayHost = relayUrl.host().isEmpty() ? relayIP : relayUrl.host();
		QHostAddress resolvedRelayAddress;
		if (!resolvedRelayAddress.setAddress(relayHost)) {
			QHostInfo info = QHostInfo::fromName(relayHost);
			if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
				LogWidget::instance()->addLog("Failed to resolve Relay IP: " + relayHost, LogWidget::Error);
				return;
			}
			// 遍历地址列表，筛选 IPv4 地址
			bool foundIPv4 = false;
			for (const QHostAddress& address : info.addresses()) {
				if (address.protocol() == QAbstractSocket::IPv4Protocol) {
					resolvedRelayAddress = address;
					foundIPv4 = true;
					break;
				}
			}
			if (!foundIPv4) {
				LogWidget::instance()->addLog("No IPv4 address found for Relay IP: " + relayHost, LogWidget::Error);
				return;
			}
		}

		saveConfig();
		m_peerClient = new PeerClient(m_uuidStr,this);

		m_peerClient->setRelayInfo(relayHost, relayPort);
		connect(m_peerClient, &PeerClient::registrationResult, this, &DeskServer::onRegistrationResult);
		connect(m_peerClient, &PeerClient::errorOccurred, this, &DeskServer::onClientError);
		m_peerClient->start(resolvedAddress, static_cast<quint16>(port));
		ui.iPLineEdit->setEnabled(false);
		ui.portLineEdit_->setEnabled(false);
		ui.iPLineEdit_3->setEnabled(false);
		ui.portLineEdit_2->setEnabled(false);
		ui.startButton_->setText("Stop");

		m_relayPeerClient = new RelayPeerClient(this);
		connect(m_relayPeerClient, &RelayPeerClient::heartbeatResponseReceived, this, [this]() {
			m_peerClient->setRelayStatus(true);
			ui.label_8->setText("Online");
			});
		connect(m_relayPeerClient, &RelayPeerClient::errorOccurred, this, [this](const QString& errorString) {
			m_peerClient->setRelayStatus(false);
			ui.label_8->setText("Offline");
			LogWidget::instance()->addLog(errorString, LogWidget::Warning);
			});
		m_relayPeerClient->start(resolvedRelayAddress, static_cast<quint16>(relayPort));
	}
	else {
		// 停止逻辑
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
		ui.iPLineEdit_3->setEnabled(true);
		ui.portLineEdit_2->setEnabled(true);
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
