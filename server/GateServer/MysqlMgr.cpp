#include "MysqlMgr.h"

MysqlMgr::MysqlMgr() {}

MysqlMgr::~MysqlMgr() {}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	//调用数据库访问对象的RegUser方法, 将注册请求转发给数据库访问对象处理, 返回结果
	return _dao.RegUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email)
{
	//调用数据库访问对象的CheckEmail方法, 将检查用户名和邮箱是否匹配的请求转发给数据库访问对象处理
	return _dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd)
{
	//调用数据库访问对象的UpdatePwd方法, 将更新密码的请求转发给数据库访问对象处理
	return _dao.UpdatePwd(name, pwd);
}
