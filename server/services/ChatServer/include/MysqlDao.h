#pragma once
// 数据库访问对象类, 用于封装对MySQL数据库的操作
//

#include "const.h"
#include <thread>
#include <mysqlx/xdevapi.h>
#include "data.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
class SqlConnection; //封装MySQL连接对象, 包含连接对象和上次操作时间(前置声明)
class MySqlPool; //MySQL连接池, 提供获取和归还连接的接口(前置声明)

class MysqlDao
{
public:
	MysqlDao(); //初始化MySQL连接池
	~MysqlDao();
	//注册用户, 返回用户ID, 如果注册失败则返回-1
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool UpdatePwd(const std::string& name, const std::string& newpwd); //更新密码
	bool CheckEmail(const std::string& name, const std::string& email); //检查用户名和邮箱是否匹配
	//检查用户名和密码是否匹配, 如果匹配则将用户信息存储在userInfo中
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	bool AddFriendApply(const int& from, const int& to, const std::string& desc,
		const std::string& back_name); //添加好友申请、申请留言与备注
	bool AddFriend(const int& from, const int& to, std::string back_name, std::string& apply_desc);

	std::shared_ptr<UserInfo> GetUser(int uid); //根据用户ID获取用户信息
	std::shared_ptr<UserInfo> GetUser(const std::string& name); //根据用户名获取用户信息
	//获取好友申请列表, 根据起始id和限制条数返回列表
	bool GetApplyList(int touid,
		std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
	long long GetOrCreatePrivateChat(int user1, int user2);
	bool GetChatThreads(int uid, long long last_thread_id, int limit,
		std::vector<ChatThreadInfo>& threads, bool& has_more);
	bool GetChatMessages(int uid, long long thread_id, long long last_message_id, int limit,
		std::vector<ChatMessageInfo>& messages, bool& has_more);
	bool SaveChatMessages(int sender_id, int recv_id, long long thread_id,
		const std::vector<std::pair<std::string, std::string>>& contents,
		std::vector<ChatMessageInfo>& messages);
	
private:
	//MySQL连接池对象, 用于管理MySQL连接, 提供线程安全的获取和归还连接的接口
	std::unique_ptr<MySqlPool> pool_;
};

//池内的一条连接: 会话对象, 最近活跃时间, 以及是否可复用的标记
class SqlConnection {
public:
	SqlConnection(std::unique_ptr<mysqlx::Session> session, int64_t lasttime) :_session(std::move(session)), _last_oper_time(lasttime) {}
	std::unique_ptr<mysqlx::Session> _session; //X DevAPI会话对象, 使用智能指针管理资源
	int64_t _last_oper_time; //上次操作时间
	bool _healthy{true}; //连接是否可复用, DAO捕获异常时置false, 归还时丢弃并补建
};

//连接池: 对外只暴露借还接口, 由检测线程保活并按池容量补足
//业务线程捕获到异常时把连接标为不可复用, 归还即销毁, 不再进入队列
class MySqlPool {
public:
	MySqlPool(const std::string& host, unsigned port, const std::string& user, const std::string& pass,
		const std::string& schema, int poolSize);

	void checkConnection(); //检查空闲连接, 并把池容量补足到 poolSize_
    bool reconnect(long long timestamp); //重新连接
	std::unique_ptr<SqlConnection> getConnection(); //获取连接
	void returnConnection(std::unique_ptr<SqlConnection> con); //归还连接
	void Close(); //关闭连接池
	~MySqlPool();

private:
	std::string host_; //MySQL服务器地址
	unsigned port_; //X Protocol端口号
	std::string user_; //MySQL用户名
	std::string pass_; //MySQL密码
	std::string schema_; //MySQL数据库名
	int poolSize_; //连接池大小

	std::queue<std::unique_ptr<SqlConnection>> pool_; //连接池队列
	std::mutex mutex_; //用于保护连接队列的互斥锁
	std::condition_variable cond_; //业务线程等待可用连接
	std::condition_variable check_cond_; //检测线程等待心跳周期、补池请求或停池通知
	bool check_requested_{true}; //补池请求标记: 启动时先补齐初始化失败的连接, 之后由丢弃连接的线程置位
	size_t connection_count_{0}; //池内连接总数(空闲+借出), 由mutex_保护; 容量判断不能用队列长度
	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	std::thread _check_thread; //检测线程, 定期检查连接的状态并保持连接的活跃性(心跳机制)
};
