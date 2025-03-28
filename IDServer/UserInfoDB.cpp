#include "UserInfoDB.h"
#include <iostream>
#include <QDateTime>
#include "LogWidget.h"

UserInfoDB::UserInfoDB(const std::string& dbPath) : db(nullptr), dbPath(dbPath) {}

UserInfoDB::~UserInfoDB() {
	close();
}

bool UserInfoDB::open() {
	if (sqlite3_open(dbPath.c_str(), &db) == SQLITE_OK) {
		std::string sql = "CREATE TABLE IF NOT EXISTS UserInfo (UUID TEXT PRIMARY KEY, IP TEXT, LastRegTime TEXT);";
		return execute(sql);
	}
	return false;
}

void UserInfoDB::close() {
	if (db) {
		sqlite3_close(db);
		db = nullptr;
	}
}

bool UserInfoDB::purgeOldRecords() {
	// 计算三天前的时间，假设存储的时间为 ISO 格式
	QString threshold = QDateTime::currentDateTime().addDays(-3).toString(Qt::ISODate);
	std::string sql = "DELETE FROM UserInfo WHERE LastRegTime IS NOT NULL AND LastRegTime < '" + threshold.toStdString() + "';";
	return execute(sql);
}

std::vector<UserInfo> UserInfoDB::getAllUserInfo() {

	// 清理三天前的记录
	purgeOldRecords();

	std::vector<UserInfo> userInfos;
	sqlite3_stmt* stmt;

	std::string sql = "SELECT UUID, IP, LastRegTime FROM UserInfo;";
	if (prepareStatement(sql, &stmt)) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			UserInfo userInfo;
			userInfo.UUID = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			userInfo.IP = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			userInfo.LastRegTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
			userInfos.push_back(userInfo);
		}
		sqlite3_finalize(stmt);
	}
	return userInfos;
}

bool UserInfoDB::createOrUpdate(const UserInfo& userInfo) {
	std::string sql = "INSERT OR REPLACE INTO UserInfo (UUID, IP, LastRegTime) VALUES (?, ?, ?);";
	sqlite3_stmt* stmt;

	if (prepareStatement(sql, &stmt)) {
		sqlite3_bind_text(stmt, 1, userInfo.UUID.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, userInfo.IP.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, userInfo.LastRegTime.c_str(), -1, SQLITE_STATIC);

		bool result = sqlite3_step(stmt) == SQLITE_DONE;
		sqlite3_finalize(stmt);
		return result;
	}
	return false;
}

bool UserInfoDB::execute(const std::string& sql) {
	char* errMsg = nullptr;
	if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
		LogWidget::instance()->addLog(QString("SQL error %1").arg(errMsg),LogWidget::Warning);
		sqlite3_free(errMsg);
		return false;
	}
	return true;
}

bool UserInfoDB::prepareStatement(const std::string& sql, sqlite3_stmt** stmt) {
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, stmt, nullptr) == SQLITE_OK) {
		return true;
	}
	LogWidget::instance()->addLog(QString("Failed to prepare statement: %1").arg(sqlite3_errmsg(db)), LogWidget::Warning);
	return false;
}