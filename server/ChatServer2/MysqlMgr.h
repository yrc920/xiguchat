#pragma once
// 用于管理MySQL连接池并提供MySQL操作接口
//

#include "MysqlDao.h"
#include "Singleton.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>; //使基类创建实例时可以访问私有构造函数
public:
	~MysqlMgr(); //使基类析构时可以访问派生类的析构函数

	//注册用户, 返回用户ID, 如果注册失败则返回-1
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email); //检查用户名和邮箱是否匹配
	bool UpdatePwd(const std::string& name, const std::string& pwd); //更新密码
	//检查邮箱和密码是否匹配, 如果匹配则将用户信息存储在userInfo中
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid); //根据用户ID获取用户信息

private:
	MysqlMgr();

	MysqlDao _dao; //数据库访问对象
};
