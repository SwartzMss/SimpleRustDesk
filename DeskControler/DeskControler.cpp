#include "DeskControler.h"
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "VideoWidget.h"
#include "LogWidget.h"

DeskControler::DeskControler(QWidget* parent)
	: QWidget(parent),
	 m_networkManager(nullptr),
	 m_videoReceiver(nullptr)
{
	ui.setupUi(this);
	// 将 LogWidget 放到界面上
	LogWidget::instance()->init(ui.widget);
	loadConfig();

	// 当点击按钮时，发起 TCP 连接并发送 PunchHoleRequest
	connect(ui.pushButton, &QPushButton::clicked,
		this, &DeskControler::onConnectClicked);
}

DeskControler::~DeskControler()
{
}


void DeskControler::loadConfig()
{
	QFile file("DeskControler.json");
	QJsonObject config;
	bool valid = false;

	if (file.exists()) {
		if (file.open(QIODevice::ReadOnly)) {
			QByteArray data = file.readAll();
			file.close();
			QJsonDocument doc = QJsonDocument::fromJson(data);
			if (!doc.isNull() && doc.isObject()) {
				config = doc.object();
				valid = true;
			}
		}
	}
	// 如果文件不存在或格式错误，则使用默认值并重写配置文件
	if (!valid) {
		config["server"] = QJsonObject{
			{"ip", "127.0.0.1"},
			{"port", 21116}
		};
		config["uuid"] = "";

		if (file.open(QIODevice::WriteOnly)) {
			QJsonDocument doc(config);
			file.write(doc.toJson());
			file.close();
		}

	}

	// 从配置中读取数据
	QJsonObject serverObj = config["server"].toObject();
	QString ip = serverObj["ip"].toString("127.0.0.1");
	int port = serverObj["port"].toInt(21116);
	QString uuid = config["uuid"].toString("");

	// 设置 UI 控件
	ui.ipLineEdit_->setText(ip);
	ui.portLineEdit_->setText(QString::number(port));
	ui.lineEdit->setText(uuid);
}

void DeskControler::saveConfig()
{
	QJsonObject config;
	QJsonObject serverObj;
	serverObj["ip"] = ui.ipLineEdit_->text().trimmed();
	serverObj["port"] = ui.portLineEdit_->text().toInt();
	config["server"] = serverObj;
	config["uuid"] = ui.lineEdit->text().trimmed();

	QJsonDocument doc(config);
	QFile file("DeskControler.json");
	if (file.open(QIODevice::WriteOnly)) {
		file.write(doc.toJson());
		file.close();
	}
}

void DeskControler::onConnectClicked()
{
	QString ip = ui.ipLineEdit_->text();
	quint16 port = ui.portLineEdit_->text().toUShort();
	QString uuid = ui.lineEdit->text();

	if (ip.isEmpty() || uuid.isEmpty() || port == 0) {
		LogWidget::instance()->addLog("please check IP / Port / Uuid", LogWidget::Warning);
		return;
	}

	saveConfig();
	LogWidget::instance()->addLog(
		QString("Attempting to connect to server at %1:%2 with UUID: %3").arg(ip).arg(port).arg(uuid),
		LogWidget::Info
	);

	m_networkManager = new NetworkManager(this);

	// 当收到 NetworkManager 的 PunchHoleResponse 信号时调用本类槽
	connect(m_networkManager, &NetworkManager::punchHoleResponseReceived,
		this, &DeskControler::onPunchHoleResponse);
	// 网络出错
	connect(m_networkManager, &NetworkManager::networkError,
		this, &DeskControler::onNetworkError);
	// 连接断开
	connect(m_networkManager, &NetworkManager::disconnected,
		this, &DeskControler::onNetworkDisconnected);

	if (!m_networkManager->connectToServer(ip, port)) {
		m_networkManager->cleanup();
		delete m_networkManager;
		m_networkManager = nullptr;
		return;
	}

	ui.ipLineEdit_->setEnabled(false);
	ui.portLineEdit_->setEnabled(false);
	ui.lineEdit->setEnabled(false);
	ui.pushButton->setEnabled(false);

	LogWidget::instance()->addLog("Server connection established. Sending punch hole request.", LogWidget::Info);
	// 连接成功后，发送 PunchHoleRequest
	m_networkManager->sendPunchHoleRequest(uuid);

}

void DeskControler::onPunchHoleResponse(const QString& relayServer, int relayPort, int result)
{
	QString resultStr;
	switch (result) {
	case 0:
		resultStr = "OK";
		break;
	case 1:
		resultStr = "ID_NOT_EXIST";
		break;
	case 2:
		resultStr = "DESKSERVER_OFFLINE";
		break;
	case 3:
		resultStr = "RELAYSERVER_OFFLINE";
		break;
	default:
		resultStr = "INNER_ERROR";
		break;
	}

	LogWidget::LogLevel logLevel = (result == 0) ? LogWidget::Info : LogWidget::Error;

	LogWidget::instance()->addLog(
		QString("Punch hole response: %1 (Relay Server: %2, Port: %3)")
		.arg(resultStr).arg(relayServer).arg(relayPort),
		logLevel
	);

	if (result != 0) {
		ui.ipLineEdit_->setEnabled(true);
		ui.portLineEdit_->setEnabled(true);
		ui.lineEdit->setEnabled(true);
		ui.pushButton->setEnabled(true);
		return;
	}

	setupVideoSession(relayServer, relayPort, resultStr);
}

void DeskControler::setupVideoSession(const QString& relayServer, quint16 relayPort, const QString& status)
{
	LogWidget::instance()->addLog(
		QString("Establishing video session via relay server %1:%2 - Status: %3").arg(relayServer).arg(relayPort).arg(status),
		LogWidget::Info
	);

	VideoWidget* videoWidget = new VideoWidget();
	videoWidget->setAttribute(Qt::WA_DeleteOnClose, true);

	QScrollArea* scrollArea = new QScrollArea(this);
	scrollArea->setWindowFlags(Qt::Window);
	scrollArea->setWidget(videoWidget);
	scrollArea->setAttribute(Qt::WA_DeleteOnClose, true);

	connect(scrollArea, &QObject::destroyed, this, [this]() {
		destroyVideoSession();
		LogWidget::instance()->addLog("Video widget closed by user.", LogWidget::Info);
		});

	scrollArea->resize(QSize(1024, 768));
	scrollArea->show();

	m_videoReceiver = new VideoReceiver(this);

	m_remoteClipboard = new RemoteClipboard(this);
	m_remoteClipboard->setRemoteWindow(scrollArea);

	if (m_remoteClipboard->start()) {
		LogWidget::instance()->addLog("Global keyboard hook installed", LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog("Failed to install global keyboard hook", LogWidget::Error);
	}
	connect(m_remoteClipboard, &RemoteClipboard::ctrlCPressed,
		m_videoReceiver, &VideoReceiver::clipboardDataCaptured);

	connect(videoWidget, &VideoWidget::mouseEventCaptured,
		m_videoReceiver, &VideoReceiver::mouseEventCaptured);

	QObject::connect(videoWidget, &VideoWidget::keyEventCaptured,
		m_videoReceiver, &VideoReceiver::keyEventCaptured);

	QObject::connect(m_videoReceiver, &VideoReceiver::onClipboardMessageReceived,
		m_remoteClipboard, &RemoteClipboard::onClipboardMessageReceived);
	

	connect(m_videoReceiver, &VideoReceiver::frameReady, this, [videoWidget, scrollArea](const QImage& img) {
		static bool firstFrame = true;
		if (firstFrame && !img.isNull())
		{
			LogWidget::instance()->addLog("Video stream started and UI initialized.", LogWidget::Info);
			QSize initialSize = img.size().expandedTo(QSize(800, 600));
			scrollArea->resize(initialSize);
			firstFrame = false;
		}
		videoWidget->setFrame(img);
		});

	QString uuid = ui.lineEdit->text();
	m_videoReceiver->startConnect(relayServer, static_cast<quint16>(relayPort), uuid);
}

void DeskControler::destroyVideoSession()
{
	if (m_remoteClipboard) {
		m_remoteClipboard->stop();
		delete m_remoteClipboard;
		m_remoteClipboard = nullptr;
	}

	ui.pushButton->setEnabled(true);
	ui.ipLineEdit_->setEnabled(true);
	ui.portLineEdit_->setEnabled(true);
	ui.lineEdit->setEnabled(true);

	if (m_videoReceiver) {
		m_videoReceiver->stopReceiving();
		delete m_videoReceiver;
		m_videoReceiver = nullptr;
	}
	if (m_networkManager) {
		m_networkManager->cleanup();
		delete m_networkManager;
		m_networkManager = nullptr;
	}
}


void DeskControler::onNetworkError(const QString& error)
{
	LogWidget::instance()->addLog(
		QString("NetworkError %1").arg(error),
		LogWidget::Warning
	);
}

void DeskControler::onNetworkDisconnected()
{
	LogWidget::instance()->addLog("Network connection disconnected.", LogWidget::Warning);
	// 如果需要，恢复按钮可点击
	ui.pushButton->setEnabled(true);
}
