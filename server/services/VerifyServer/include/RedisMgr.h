#pragma once
// 用于管理 Redis 连接池并提供验证码读写接口
//

#include "const.h"
#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
class RedisConPool; //连接池类, 用于管理 Redis 连接(前置声明)

class RedisMgr : public Singleton<RedisMgr>,
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>; //使基类创建实例时可以访问私有构造函数
public:
	~RedisMgr(); //使基类析构时可以访问派生类的析构函数

	//获取 key 对应的 value，key 不存在或执行失败返回 false
	bool Get(const std::string& key, std::string& value);
	//设置 key 的 value 并同时指定过期时间(秒)，单条 SETEX 命令，写入与设过期是原子操作
	bool SetExpire(const std::string& key, const std::string& value, int expire_seconds);

	void Close(); //关闭连接池, 释放资源

private:
	RedisMgr();
	//连接池对象, 用于管理 Redis 连接, 提供线程安全的获取和归还连接的接口
	std::shared_ptr<RedisConPool> _con_pool;
};

class RedisConPool {
public:
	RedisConPool(size_t poolSize, const char* host, int port, const char* pwd);
	~RedisConPool();
	//从连接池中获取一个可用的 redisContext 对象, 如果连接池为空则等待直到有连接可用
	redisContext* getConnection();
	redisContext* getConNonBlock(); //非阻塞获取连接, 如果连接池为空则返回 nullptr
	//将使用完的 redisContext 对象返回连接池, 使其可供其他线程使用
	void returnConnection(redisContext* context);
	void Close(); //标记连接池停止, 唤醒所有等待的线程, 使其退出

private:
	void checkThread(); //检测线程函数, 定期检查连接的状态并保持连接的活跃性(心跳机制)
	bool reconnect(); //重新连接

	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	size_t poolSize_; //连接池的大小
	std::string host_; //Redis 服务器的主机地址
	std::string pwd_; //Redis 服务器的密码
	int port_; //Redis 服务器的端口号
	std::queue<redisContext*> connections_; //连接池中的连接队列
	std::mutex mutex_; //保护连接队列的互斥锁
	std::condition_variable cond_; //连接池为空时等待、有连接归还时通知
	std::condition_variable check_cond_; //唤醒检测线程检查容量缺口或停止状态
	size_t connection_count_{0}; //空闲与借出连接总数
	bool check_requested_{true}; //补池请求标记: 启动时先补齐未建成的连接, 之后由归还坏连接的线程置位
	std::thread check_thread_; //检测线程, 定期检查连接的状态并保持连接的活跃性(心跳机制)
};
