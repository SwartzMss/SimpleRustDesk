#include "DeskControler.h"
#include "NetworkManager.h"
#include "VideoReceiver.h"   // 【MOD】包含 VideoReceiver
#include "VideoWidget.h"     // 【MOD】包含 VideoWidget
#include <QMessageBox>
#include "LogWidget.h"

DeskControler::DeskControler(QWidget* parent)
	: QWidget(parent),
	networkManager(new NetworkManager(this))
{
	ui.setupUi(this);
	LogWidget::instance()->init(ui.widget);

	connect(ui.pushButton, &QPushButton::clicked, this, &DeskControler::onConnectClicked);

	connect(networkManager, &NetworkManager::punchHoleResponseReceived,
		this, &DeskControler::onPunchHoleResponse);
	connect(networkManager, &NetworkManager::networkError,
		this, &DeskControler::onNetworkError);
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
		LogWidget::instance()->addLog(QString("please check IP Port and Uuid"), LogWidget::Info);
		return;
	}
	if (!networkManager->connectToServer(ip, port))
	{
		LogWidget::instance()->addLog(QString("connect Server failed"), LogWidget::Warning);
		return;
	}

	networkManager->sendPunchHoleRequest(uuid);
	ui.pushButton->setEnabled(false);
}

void DeskControler::onPunchHoleResponse(const QString& relayServer, int relayPort, int result)
{
	if (result == 0) {
		// 记录 relay 信息
		LogWidget::instance()->addLog(QString("Relay Server: %1\nRelay Port: %2\nResult: OK")
			.arg(relayServer).arg(relayPort), LogWidget::Info);
		// 【MOD】启动视频接收流程
		// 获取用户输入的 uuid
		QString uuid = ui.lineEdit->text();

		// 创建 VideoReceiver 和 VideoWidget 实例
		VideoReceiver* videoReceiver = new VideoReceiver(); // 无父对象，由应用管理生命周期
		VideoWidget* videoWidget = new VideoWidget();         // 独立窗口
		// 将解码后的帧显示到 VideoWidget
		connect(videoReceiver, &VideoReceiver::frameReady, videoWidget, &VideoWidget::setFrame);

		// 显示 VideoWidget（它会根据接收到的帧调整窗口大小）
		videoWidget->show();

		// 连接到 relay 服务器，并在连接后自动发送 RequestRelay 消息
		videoReceiver->connectToServer(relayServer, static_cast<quint16>(relayPort), uuid);
	}
	else {
		QString resultStr;
		switch (result) {
		case 1:
			resultStr = "ID_NOT_EXIST";
			break;
		case 2:
			resultStr = "OFFLINE";
			break;
		default:
			resultStr = "UNKNOWN";
			break;
		}
		LogWidget::instance()->addLog(QString("Relay Server: %1\nRelay Port: %2\nResult: %3")
			.arg(relayServer).arg(relayPort).arg(resultStr), LogWidget::Info);
	}
}

void DeskControler::onNetworkError(const QString& error)
{
	LogWidget::instance()->addLog(QString("NetworkError %1").arg(error), LogWidget::Warning);
}

void DeskControler::onNetworkDisconnected()
{
	LogWidget::instance()->addLog(QString("Network Disconnected"), LogWidget::Info);
}
