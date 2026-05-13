#pragma once
// 数据库访问对象类, 用于封装对MySQL数据库的操作
//

#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include "data.h"
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
class SqlConnection; //封装MySQL连接对象, 包含连接对象和上次操作时间(前置声明)
class MySqlPool; //MySQL连接池, 提供获取和归还连接的接口(前置声明)

class MysqlDao
{
public:
	MysqlDao(); //初始化MySQL连接池
	//注册用户, 返回用户ID, 如果注册失败则返回-1
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool UpdatePwd(const std::string& name, const std::string& newpwd); //更新密码
	bool CheckEmail(const std::string& name, const std::string& email); //检查用户名和邮箱是否匹配
	//检查用户名和密码是否匹配, 如果匹配则将用户信息存储在userInfo中
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid); //根据用户ID获取用户信息
	~MysqlDao();

private:
	//MySQL连接池对象, 用于管理MySQL连接, 提供线程安全的获取和归还连接的接口
	std::unique_ptr<MySqlPool> pool_;
};

class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}
	std::unique_ptr<sql::Connection> _con; //MySQL连接对象, 使用智能指针管理资源
	int64_t _last_oper_time; //上次操作时间
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, 
		const std::string& schema, int poolSize);

	void checkConnection(); //检查连接的有效性
	std::unique_ptr<SqlConnection> getConnection(); //获取连接
	void returnConnection(std::unique_ptr<SqlConnection> con); //归还连接
	void Close(); //关闭连接池
	~MySqlPool();

private:
	std::string url_; //MySQL服务器的URL地址, 包含主机地址和端口号
	std::string user_; //MySQL用户名
	std::string pass_; //MySQL密码
	std::string schema_; //MySQL数据库名
	int poolSize_; //连接池大小

	std::queue<std::unique_ptr<SqlConnection>> pool_; //连接池队列
	std::mutex mutex_; //用于保护连接队列的互斥锁
	std::condition_variable cond_; //用于线程间的条件变量, 当连接池为空时等待, 当有连接可用时通知
	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	std::thread _check_thread; //检测线程, 定期检查连接的状态并保持连接的活跃性(心跳机制)
};
