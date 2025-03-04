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
	// 清空现有数据
	model->clear();

	// 设置表头
	model->setHorizontalHeaderLabels({ "ID", "UUID", "IP", "Status" });

	// 获取所有用户信息
	std::vector<UserInfo> userInfos = userInfoDB.getAllUserInfo();

	// 将数据加载到 model 中
	for (const auto& userInfo : userInfos) {
		QList<QStandardItem*> items;
		items.append(new QStandardItem(QString::number(userInfo.ID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.UUID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.IP)));
		items.append(new QStandardItem(QString::fromStdString("Offline")));
		model->appendRow(items);
	}

	// 调整列宽
	ui.tableView->resizeColumnsToContents();

	connect(timer, &QTimer::timeout, this, &IDServer::updateOnlineStatus);
	timer->start(5000);
	
}

void IDServer::onStopClicked()
{
	if (timer->isActive()) {
		timer->stop();  // 停止定时器
	}

	userInfoDB.close();
}

void IDServer::updateOnlineStatus()
{
	for (int row = 0; row < model->rowCount(); ++row) {
		// 获取 IP 地址
		QStandardItem* ipItem = model->item(row, 2);  // IP 列是第 3 列（索引 2）
		QString ip = ipItem->text();

		// 检测在线状态（假设有一个函数 checkOnlineStatus）
		bool isOnline = true;

		// 更新 onlineStatus 列
		QStandardItem* statusItem = model->item(row, 3);  // Online Status 列是第 4 列（索引 3）
		statusItem->setText(isOnline ? "Online" : "Offline");
	}
}
