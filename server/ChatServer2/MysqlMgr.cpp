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

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	//调用数据库访问对象的CheckPwd方法, 将检查邮箱和密码是否匹配的请求转发给数据库访问对象处理
	return _dao.CheckPwd(email, pwd, userInfo);
}

bool MysqlMgr::AddFriendApply(const int& from, const int& to)
{
	//调用数据库访问对象的AddFriendApply方法, 将添加好友申请的请求转发给数据库访问对象处理
	return _dao.AddFriendApply(from, to);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
	//调用数据库访问对象的GetUser方法, 将根据用户ID获取用户信息的请求转发给数据库访问对象处理
	return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(const std::string& name)
{
	//调用数据库访问对象的GetUser方法, 将根据用户名获取用户信息的请求转发给数据库访问对象处理
	return _dao.GetUser(name);
}

bool MysqlMgr::GetApplyList(int touid,
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
	//调用数据库访问对象的GetApplyList方法, 将获取好友申请列表的请求转发给数据库访问对象处理
	return _dao.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info)
{
	return _dao.GetFriendList(self_id, user_info);
}
