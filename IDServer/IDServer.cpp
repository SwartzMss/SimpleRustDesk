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

	ui.tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableView->horizontalHeader()->setStretchLastSection(true);

	timer = new QTimer(this);

	server_ = new RendezvousServer;

	ui.stopButton_->setEnabled(false);
}

IDServer::~IDServer()
{
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
	// 清空现有数据
	model->clear();

	// 设置表头
	model->setHorizontalHeaderLabels({"UUID", "IP", "Status" });

	// 获取所有用户信息
	std::vector<UserInfo> userInfos = userInfoDB.getAllUserInfo();

	// 将数据加载到 model 中
	for (const auto& userInfo : userInfos) {
		QList<QStandardItem*> items;
		items.append(new QStandardItem(QString::fromStdString(userInfo.UUID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.IP)));
		items.append(new QStandardItem(QString::fromStdString("Offline")));
		model->appendRow(items);
	}

	// 调整列宽
	ui.tableView->resizeColumnsToContents();

	connect(timer, &QTimer::timeout, this, &IDServer::updateOnlineStatus);
	timer->start(5000);

	ui.startButton_->setEnabled(false);
	ui.stopButton_->setEnabled(true);
	ui.lineEdit->setEnabled(false);


	
	
}

void IDServer::onStopClicked()
{
	if (timer->isActive()) {
		timer->stop();  // 停止定时器
	}

	userInfoDB.close();

	ui.startButton_->setEnabled(true);
	ui.stopButton_->setEnabled(false);
	ui.lineEdit->setEnabled(true);
	server_->stop();
}

void IDServer::updateOnlineStatus()
{
	for (int row = 0; row < model->rowCount(); ++row) {
		// 获取 IP 地址
		QStandardItem* ipItem = model->item(row, 1);
		QString ip = ipItem->text();

		// 检测在线状态（假设有一个函数 checkOnlineStatus）
		bool isOnline = true;

		// 更新 onlineStatus 列
		QStandardItem* statusItem = model->item(row, 2); 
		statusItem->setText(isOnline ? "Online" : "Offline");
	}
}
