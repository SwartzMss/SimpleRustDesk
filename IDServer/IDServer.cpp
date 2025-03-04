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
	// �����������
	model->clear();

	// ���ñ�ͷ
	model->setHorizontalHeaderLabels({ "ID", "UUID", "IP", "Status" });

	// ��ȡ�����û���Ϣ
	std::vector<UserInfo> userInfos = userInfoDB.getAllUserInfo();

	// �����ݼ��ص� model ��
	for (const auto& userInfo : userInfos) {
		QList<QStandardItem*> items;
		items.append(new QStandardItem(QString::number(userInfo.ID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.UUID)));
		items.append(new QStandardItem(QString::fromStdString(userInfo.IP)));
		items.append(new QStandardItem(QString::fromStdString("Offline")));
		model->appendRow(items);
	}

	// �����п�
	ui.tableView->resizeColumnsToContents();

	connect(timer, &QTimer::timeout, this, &IDServer::updateOnlineStatus);
	timer->start(5000);
	
}

void IDServer::onStopClicked()
{
	if (timer->isActive()) {
		timer->stop();  // ֹͣ��ʱ��
	}

	userInfoDB.close();
}

void IDServer::updateOnlineStatus()
{
	for (int row = 0; row < model->rowCount(); ++row) {
		// ��ȡ IP ��ַ
		QStandardItem* ipItem = model->item(row, 2);  // IP ���ǵ� 3 �У����� 2��
		QString ip = ipItem->text();

		// �������״̬��������һ������ checkOnlineStatus��
		bool isOnline = true;

		// ���� onlineStatus ��
		QStandardItem* statusItem = model->item(row, 3);  // Online Status ���ǵ� 4 �У����� 3��
		statusItem->setText(isOnline ? "Online" : "Offline");
	}
}
