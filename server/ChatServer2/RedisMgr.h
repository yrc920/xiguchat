#pragma once
// 用于管理Redis连接池并提供Redis操作接口
//

#include "const.h"
#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include "Singleton.h"
class RedisConPool; //连接池类, 用于管理Redis连接(前置声明)

class RedisMgr : public Singleton<RedisMgr>,
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>; //使基类创建实例时可以访问私有构造函数
public:
	~RedisMgr(); //使基类析构时可以访问派生类的析构函数

	bool Get(const std::string& key, std::string& value); //获取key对应的value
	bool Set(const std::string& key, const std::string& value); //设置key和value
	bool Del(const std::string& key); //删除key
	bool ExistsKey(const std::string& key); //检查key是否存在

	bool LPush(const std::string& key, const std::string& value); //将value插入到key对应的列表的头部
	bool LPop(const std::string& key, std::string& value); //移除并返回key对应的列表的头部元素
	bool RPush(const std::string& key, const std::string& value); //将value插入到key对应的列表的尾部
	bool RPop(const std::string& key, std::string& value); //移除并返回key对应的列表的尾部元素

	//设置哈希表key中字段hkey的值(二级哈希缓存)
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	//设置哈希表key中字段hkey的值，支持二进制数据
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	//获取哈希表key中字段hkey的值(二级哈希缓存)
	std::string HGet(const std::string& key, const std::string& hkey);
	
	void Close(); //关闭连接池, 释放资源

private:
	RedisMgr();
	//连接池对象, 用于管理Redis连接, 提供线程安全的获取和归还连接的接口
	std::shared_ptr<RedisConPool> _con_pool;
};

class RedisConPool {
public:
	RedisConPool(size_t poolSize, const char* host, int port, const char* pwd);
	~RedisConPool();
	//从连接池中获取一个可用的redisContext对象, 如果连接池为空则等待直到有连接可用
	redisContext* getConnection();
	//将使用完的redisContext对象返回连接池, 使其可供其他线程使用
	void returnConnection(redisContext* context);
	void Close(); //标记连接池停止, 唤醒所有等待的线程, 使其退出

private:
	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	size_t poolSize_; //连接池的大小
	const char* host_; //Redis服务器的主机地址
	int port_; //Redis服务器的端口号
	std::queue<redisContext*> connections_; //连接池中的连接队列, 存储可用的redisContext对象
	std::mutex mutex_; //用于保护连接队列的互斥锁
	std::condition_variable cond_; //用于线程间的条件变量, 当连接池为空时等待, 当有连接可用时通知
};
