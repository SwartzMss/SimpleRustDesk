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
		LogWidget::instance()->addLog("please check IP / Port / Uuid", LogWidget::Info);
		return;
	}
	ui.pushButton->setEnabled(false);

	// 连接到服务端（阻塞 3 秒等结果），失败则提示
	if (!networkManager->connectToServer(ip, port)) {
		LogWidget::instance()->addLog("connect Server failed", LogWidget::Warning);
		ui.pushButton->setEnabled(true);
		return;
	}
	// 连接成功后，发送 PunchHoleRequest
	networkManager->sendPunchHoleRequest(uuid);

}

void DeskControler::onPunchHoleResponse(const QString& relayServer, int relayPort, int result)
{
	// 显示响应结果
	QString resultStr;
	switch (result) {
	case 1:  resultStr = "ID_NOT_EXIST"; break;
	case 2:  resultStr = "DESKSERVER_OFFLINE";break;
	case 3:  resultStr = "RELAYSERVER_OFFLINE"; break;
	default: resultStr = "INNER_ERROR";break;
	}

	if (result != 0) {
		LogWidget::instance()->addLog(
			QString("PunchHoleResponse failed, Result: %3").arg(resultStr),
			LogWidget::Info
		);
		ui.ipLineEdit_->setEnabled(true);
		ui.portLineEdit_->setEnabled(true);
		ui.lineEdit->setEnabled(true);
		ui.pushButton->setEnabled(true);
		return;
	}

	if (result == 0) {  // OK
		LogWidget::instance()->addLog(
			QString("Relay Server: %1\nRelay Port: %2\nResult: OK")
			.arg(relayServer).arg(relayPort),
			LogWidget::Info
		);
		// 获取 uuid
		QString uuid = ui.lineEdit->text();

		// 创建 VideoReceiver 和 VideoWidget，用来连接 Relay 并显示视频
		VideoReceiver* videoReceiver = new VideoReceiver();  // 由应用管理生命周期
		VideoWidget* videoWidget = new VideoWidget();     // 独立窗口
		videoWidget->setAttribute(Qt::WA_DeleteOnClose, true);


		QScrollArea* scrollArea = new QScrollArea();
		scrollArea->setWidget(videoWidget);
		scrollArea->setAttribute(Qt::WA_DeleteOnClose, true);


		// 当解码出帧时，发送到 videoWidget 显示
		connect(videoReceiver, &VideoReceiver::frameReady, this, [videoWidget, scrollArea](const QImage& img) {

			static bool firstFrame = true;
			if (firstFrame && !img.isNull()) 
			{
				QSize initialSize = img.size().expandedTo(QSize(800, 600));
				scrollArea->resize(initialSize);
				firstFrame = false;
				scrollArea->show();
			}
			videoWidget->setFrame(img);

			});


		connect(scrollArea, &QObject::destroyed, [this, videoReceiver]() {
			videoReceiver->deleteLater();
			ui.ipLineEdit_->setEnabled(true);
			ui.portLineEdit_->setEnabled(true);
			ui.lineEdit->setEnabled(true);
			ui.pushButton->setEnabled(true);
			});

		ui.ipLineEdit_->setEnabled(false);
		ui.portLineEdit_->setEnabled(false);
		ui.lineEdit->setEnabled(false);
		ui.pushButton->setEnabled(false);


		// 发起连接到 Relay 服务器
		videoReceiver->startConnect(relayServer, static_cast<quint16>(relayPort), uuid);
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
	LogWidget::instance()->addLog("Network Disconnected", LogWidget::Info);
	// 如果需要，恢复按钮可点击
	ui.pushButton->setEnabled(true);
}
