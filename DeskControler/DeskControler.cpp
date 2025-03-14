#include "DeskControler.h"
#include "NetworkManager.h"
#include <QScrollArea>
#include "VideoReceiver.h"
#include "VideoWidget.h"
#include <QMessageBox>
#include "LogWidget.h"

DeskControler::DeskControler(QWidget* parent)
	: QWidget(parent),
	networkManager(new NetworkManager(this))
{
	ui.setupUi(this);
	// 将 LogWidget 放到界面上
	LogWidget::instance()->init(ui.widget);

	// 当点击按钮时，发起 TCP 连接并发送 PunchHoleRequest
	connect(ui.pushButton, &QPushButton::clicked,
		this, &DeskControler::onConnectClicked);

	// 当收到 NetworkManager 的 PunchHoleResponse 信号时调用本类槽
	connect(networkManager, &NetworkManager::punchHoleResponseReceived,
		this, &DeskControler::onPunchHoleResponse);
	// 网络出错
	connect(networkManager, &NetworkManager::networkError,
		this, &DeskControler::onNetworkError);
	// 连接断开
	connect(networkManager, &NetworkManager::disconnected,
		this, &DeskControler::onNetworkDisconnected);
}

DeskControler::~DeskControler()
{
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
	ui.pushButton->setEnabled(false);

	LogWidget::instance()->addLog(
		QString("Attempting to connect to server at %1:%2 with UUID: %3").arg(ip).arg(port).arg(uuid),
		LogWidget::Info
	);
	// 连接到服务端（阻塞 3 秒等结果），失败则提示
	if (!networkManager->connectToServer(ip, port)) {
		LogWidget::instance()->addLog(
			QString("Failed to connect to server at %1:%2").arg(ip).arg(port),
			LogWidget::Error
		);
		ui.pushButton->setEnabled(true);
		return;
	}
	LogWidget::instance()->addLog("Server connection established. Sending punch hole request.", LogWidget::Info);
	// 连接成功后，发送 PunchHoleRequest
	networkManager->sendPunchHoleRequest(uuid);

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

	VideoReceiver* videoReceiver = new VideoReceiver(this);
	VideoWidget* videoWidget = new VideoWidget();
	videoWidget->setAttribute(Qt::WA_DeleteOnClose, true);

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setWidget(videoWidget);
	scrollArea->setAttribute(Qt::WA_DeleteOnClose, true);

	// 当解码出帧时，发送到 videoWidget 显示
	connect(videoReceiver, &VideoReceiver::frameReady, this, [videoWidget, scrollArea](const QImage& img) {

		static bool firstFrame = true;
		if (firstFrame && !img.isNull())
		{
			LogWidget::instance()->addLog("Video stream started and UI initialized.", LogWidget::Info);
			QSize initialSize = img.size().expandedTo(QSize(800, 600));
			scrollArea->resize(initialSize);
			firstFrame = false;
			scrollArea->show();
		}
		videoWidget->setFrame(img);

		});

	connect(scrollArea, &QObject::destroyed, this, [this]() {
		ui.pushButton->setEnabled(true);
		ui.ipLineEdit_->setEnabled(true);
		ui.portLineEdit_->setEnabled(true);
		ui.lineEdit->setEnabled(true);
		LogWidget::instance()->addLog("Video widget closed by user.", LogWidget::Info);
		});

	ui.ipLineEdit_->setEnabled(false);
	ui.portLineEdit_->setEnabled(false);
	ui.lineEdit->setEnabled(false);
	ui.pushButton->setEnabled(false);

	QString uuid = ui.lineEdit->text();
	videoReceiver->startConnect(relayServer, static_cast<quint16>(relayPort), uuid);
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
