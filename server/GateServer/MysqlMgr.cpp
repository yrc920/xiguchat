#include "MysqlMgr.h"

MysqlMgr::MysqlMgr() {}

MysqlMgr::~MysqlMgr() {}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	//调用数据库访问对象的RegUser方法, 将注册请求转发给数据库访问对象处理, 返回结果
	return _dao.RegUser(name, email, pwd);
}
