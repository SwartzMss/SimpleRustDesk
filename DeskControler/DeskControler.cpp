#include "DeskControler.h"
#include "NetworkManager.h"
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
	QString resultStr;
	switch (result) {
	case 0:
		resultStr = "OK";
		break;
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

void DeskControler::onNetworkError(const QString& error)
{
	LogWidget::instance()->addLog(QString("NetworkError %1").arg(error), LogWidget::Warning);
}

void DeskControler::onNetworkDisconnected()
{
	LogWidget::instance()->addLog(QString("Network Disconnected"), LogWidget::Info);
}
