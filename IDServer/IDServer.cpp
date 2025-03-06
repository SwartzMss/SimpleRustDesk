#include "IDServer.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QMessageBox>
#include <QtSql/QSqlQuery>
#include "LogWidget.h"

IDServer::IDServer(QWidget* parent)
	: QWidget(parent), userInfoDB("userinfo.db"), model(new QStandardItemModel(this))
{
	ui.setupUi(this);
	LogWidget::instance()->init(ui.logWidget_);

	connect(ui.startButton_, &QPushButton::clicked, this, &IDServer::onStartClicked);
	connect(ui.stopButton_, &QPushButton::clicked, this, &IDServer::onStopClicked);
	ui.tableView->setModel(model);

	server_ = new RendezvousServer;

	ui.stopButton_->setEnabled(false);

	connect(server_, &RendezvousServer::registrationSuccess, this, &IDServer::onRegistrationSuccess);
	connect(server_, &RendezvousServer::connectionDisconnected, this, &IDServer::onConnectionDisconnected);
}

IDServer::~IDServer()
{
}


void IDServer::onRegistrationSuccess(const QString& uuid, const QString& ip) 
{
	UserInfo userInfo;
	userInfo.UUID = uuid.toStdString();
	userInfo.IP = ip.toStdString();
	userInfo.LastRegTime = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
	LogWidget::instance()->addLog( QString("Registration successful: %1, IP %2 ").arg(uuid).arg(ip), LogWidget::Info);
	if (!userInfoDB.createOrUpdate(userInfo)) {
		LogWidget::instance()->addLog(QString("failed to write to database").arg(uuid).arg(ip), LogWidget::Error);
	}

	// 更新UI的表格，先判断是否已存在该 uuid
	bool exist = false;
	for (int row = 0; row < model->rowCount(); ++row) {
		QStandardItem* item = model->item(row, 0);
		if (item && item->text() == uuid) {
			// 如果已存在则更新状态为 Online
			QStandardItem* statusItem = model->item(row, 3);
			statusItem->setText("Online");
			exist = true;
			break;
		}
	}
	// 如果不存在，则添加新行
	if (!exist) {
		QList<QStandardItem*> items;
		items.append(new QStandardItem(uuid));
		items.append(new QStandardItem(ip));
		items.append(new QStandardItem(QString::fromStdString(userInfo.LastRegTime)));
		items.append(new QStandardItem("Online"));
		model->appendRow(items);
	}
	ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
}


void IDServer::onConnectionDisconnected(const QString& uuid)
{
	for (int row = 0; row < model->rowCount(); ++row) {
		QStandardItem* uuidItem = model->item(row, 0);
		if (uuidItem && uuidItem->text() == uuid) {
			QStandardItem* statusItem = model->item(row, 3);
			if (statusItem) {
				statusItem->setText("Offline");
			}
			break;
		}
	}
}

void IDServer::onStartClicked()
{
	if (!userInfoDB.open()) {
		LogWidget::instance()->addLog(QString("Failed to open database"), LogWidget::Error);
		return;
	}

	QString text = ui.lineEdit->text();
	bool isNumber = false;
	int port = text.toInt(&isNumber);
	if (!isNumber)
	{
		LogWidget::instance()->addLog("Port is invalid", LogWidget::Error);
		return;
	}

	if (!server_->start(port))
	{
		return;
	}
	LogWidget::instance()->addLog("Server start successfully", LogWidget::Info);

	ui.startButton_->setEnabled(false);
	ui.stopButton_->setEnabled(true);
	ui.lineEdit->setEnabled(false);

	// 获取所有用户信息
	std::vector<UserInfo> userInfos = userInfoDB.getAllUserInfo();

	if (userInfos.size() == 0)
	{
		return;
	}

	// 清空现有数据
	model->clear();

	// 设置表头
	model->setHorizontalHeaderLabels({ "UUID", "IP",  "LastRegTime","Status" });
	// 将数据加载到 model 中
	for (const auto& userInfo : userInfos) {
		QList<QStandardItem*> items;
		items.append(new QStandardItem(QString::fromStdString(userInfo.UUID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.IP)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.LastRegTime)));
		items.append(new QStandardItem(QString::fromStdString("Offline")));
		model->appendRow(items);
	}

	ui.tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui.tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);


}

void IDServer::onStopClicked()
{
	userInfoDB.close();

	ui.startButton_->setEnabled(true);
	ui.stopButton_->setEnabled(false);
	ui.lineEdit->setEnabled(true);
	server_->stop();
	model->clear();
}
