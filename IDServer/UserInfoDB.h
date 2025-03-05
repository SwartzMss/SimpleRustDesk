#ifndef USERINFODB_H
#define USERINFODB_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct UserInfo {
	std::string UUID;
	std::string IP;
};

class UserInfoDB {
public:
	UserInfoDB(const std::string& dbPath);
	~UserInfoDB();

	bool open();
	void close();

	std::vector<UserInfo> getAllUserInfo();
	bool createOrUpdate(const UserInfo& userInfo);

private:
	sqlite3* db;
	std::string dbPath;

	bool execute(const std::string& sql);
	bool prepareStatement(const std::string& sql, sqlite3_stmt** stmt);
};

#endif // USERINFODB_H