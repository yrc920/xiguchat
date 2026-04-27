#pragma once
// 用于管理MySQL连接池并提供MySQL操作接口
//

#include "const.h"
#include "MysqlDao.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>; //使基类创建实例时可以访问私有构造函数
public:
	~MysqlMgr(); //使基类析构时可以访问派生类的析构函数

	//注册用户, 返回用户ID, 如果注册失败则返回-1
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);

private:
	MysqlMgr();

	MysqlDao  _dao; //数据库访问对象
};
