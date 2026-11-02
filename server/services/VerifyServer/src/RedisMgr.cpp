#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::RedisMgr()
{
	//从配置管理器中获取 Redis 服务器的主机地址、端口号和密码
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	//初始化连接池, 连接池大小为 5
	_con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的 redisContext 对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回 false
	if (connect == nullptr) {
		return false;
	}

	//执行 redis 命令行, 获取 key 对应的 value
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());

	//返回 NULL 或非字符串类型都视为失败(key 不存在时为 REDIS_REPLY_NIL)
	if (reply == nullptr || reply->type != REDIS_REPLY_STRING) {
		std::cout << "Execut command [ GET " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放 redisCommand 返回的 redisReply 所占内存
		_con_pool->returnConnection(connect); //将使用完的连接归还连接池
		return false;
	}

	value = reply->str; //将获取到的 value 赋值给传出参数
	std::cout << "Execut command [ GET " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

bool RedisMgr::SetExpire(const std::string& key, const std::string& value, int expire_seconds)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的 redisContext 对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回 false
	if (connect == nullptr) {
		return false;
	}

	//SETEX 在一条命令内完成"写入 + 设过期时间", 不存在写入成功但过期时间丢失的中间态
	auto reply = (redisReply*)redisCommand(connect, "SETEX %s %d %s",
		key.c_str(), expire_seconds, value.c_str());

	if (reply == nullptr) {
		std::cout << "Execut command [ SETEX " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}
	//SETEX 成功返回状态 OK
	if (!(reply->type == REDIS_REPLY_STATUS &&
		(strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0))) {
		std::cout << "Execut command [ SETEX " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	std::cout << "Execut command [ SETEX " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect);
	return true;
}

void RedisMgr::Close()
{
	_con_pool->Close(); //标记连接池停止, 唤醒所有等待连接的线程
}

RedisMgr::~RedisMgr()
{
	Close();
}

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
	: b_stop_(false), poolSize_(poolSize), host_(host), pwd_(pwd), port_(port) {
	//创建 poolSize 条连接；连接参数只在建立时使用，之后由队列管理
	for (size_t i = 0; i < poolSize; ++i) {
		auto* context = redisConnect(host, port); //连接 Redis 服务器
		//连接失败则释放该连接并继续尝试下一个
		if (context == nullptr || context->err != 0) {
			if (context != nullptr)
				redisFree(context); //释放连接对象

			continue;
		}

		//执行 AUTH 命令进行密码认证
		auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
	    //连接中断时 redisCommand 可能返回空指针；只有收到 OK 状态才将连接加入池中，其他情况释放连接
		if (!reply || reply->type != REDIS_REPLY_STATUS || !reply->str || std::strcmp(reply->str, "OK") != 0) {
			std::cout << "Authentication failure" << std::endl;
			freeReplyObject(reply); //释放 redisCommand 返回的 redisReply 所占内存
			redisFree(context); //释放连接对象
			continue;
		}

		freeReplyObject(reply); //释放 redisCommand 返回的 redisReply 所占内存
		std::cout << "Authentication successful" << std::endl;
		connections_.push(context); //将连接加入队列, 供其他线程使用
		++connection_count_; //增加连接池中连接的数量
	}

    //启动检测线程, 定期检查连接的状态并保持连接的活跃性(心跳机制)
	check_thread_ = std::thread([this]() {
		while (!b_stop_) {
			//锁只覆盖等待本身, 检查与重连全部在锁外执行
			{
				std::unique_lock<std::mutex> lock(mutex_);
				//两个唤醒来源: 停机, 或业务线程归还坏连接后请求补池; 谓词在锁内求值, 不会漏掉通知
				check_cond_.wait_for(lock, std::chrono::seconds(60), [this] {
					return b_stop_.load() || check_requested_;
				});
				if (b_stop_) break;
				check_requested_ = false; //在锁内清除, 与置位方互斥, 本轮未处理的缺口仍会在本轮补足
			}
			checkThread(); //检查连接池中的连接是否可用, 如果不可用则尝试重新连接
		}
	});
}

redisContext* RedisConPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
    //使用条件变量等待连接池中有可用的连接或者连接池被标记为停止
    cond_.wait(lock, [this] {
        //如果连接池被标记为停止, 则直接返回true, 保持锁定状态并继续执行
        if (b_stop_)
            return true;
        //如果没有可用的连接, 返回false, 解锁并进入等待状态;
        //如果有可用的连接, 返回true, 保持锁定状态并继续执行
        return !connections_.empty();
        });
    //如果停止则直接返回空指针
    if (b_stop_)
        return  nullptr;

    //从连接池中获取一个可用的redisContext对象
    auto* context = connections_.front();
    connections_.pop(); //从连接池中移除获取的redisContext对象
    return context;
}

redisContext* RedisConPool::getConNonBlock()
{
	std::lock_guard<std::mutex> lock(mutex_);
    //如果连接池被标记为停止或者连接池为空, 则直接返回空指针
	if (b_stop_ || connections_.empty())
	    return nullptr;
	auto* context = connections_.front();
	connections_.pop();
	return context;
}

void RedisConPool::checkThread()
{
	size_t count;
	{
	    std::lock_guard<std::mutex> lock(mutex_);
	    count = connections_.size();
	}

	for (size_t i = 0; i < count && !b_stop_; ++i) {
		//从连接池中获取一个连接，如果连接池为空则返回 nullptr
		auto* context = getConNonBlock();
		if (!context)
		    break;
	    //执行 PING 命令检查连接是否可用
		auto* reply = static_cast<redisReply*>(redisCommand(context, "PING"));
	    //检查连接是否健康：没有错误，返回类型为状态，返回值为 PONG
		const bool healthy = context->err == 0 && reply && reply->type == REDIS_REPLY_STATUS && reply->str && std::strcmp(reply->str, "PONG") == 0;
	    //释放 reply 对象占用的内存
		if (reply)
		    freeReplyObject(reply);
	    //如果连接健康，则将连接归还连接池
		if (healthy)
		    returnConnection(context);
		else
		{
		    redisFree(context); //释放不健康的连接对象
		    std::lock_guard<std::mutex> lock(mutex_);
		    --connection_count_; //减少连接池中连接对象的数量
		}
	}

	for (;;) {
		{
		    std::lock_guard<std::mutex> lock(mutex_);
		    //如果连接池被标记为停止或者连接池中连接对象的数量已经达到最大值, 则直接返回, 不再尝试重新连接
		    if (b_stop_ || connection_count_ >= poolSize_)
		        return;
		}
		if (!reconnect())
		    return;
	}
}

bool RedisConPool::reconnect()
{
	auto* context = redisConnect(host_.c_str(), port_); //连接 Redis 服务器
	if (!context || context->err)
	{
	    //如果连接失败，释放连接对象并返回 false
	    if (context)
	        redisFree(context);
	    return false;
	}

    //执行 AUTH 命令进行密码认证
	auto* reply = static_cast<redisReply*>(redisCommand(context, "AUTH %s", pwd_.c_str()));
    //连接中断时 redisCommand 可能返回空指针；只有收到 OK 状态才将连接加入池中，其他情况释放连接
	const bool authenticated = reply && reply->type == REDIS_REPLY_STATUS && reply->str && std::strcmp(reply->str, "OK") == 0;
	if (reply)
	    freeReplyObject(reply); //释放 redisCommand 返回的 redisReply 所占内存
	if (!authenticated)
	{
	    redisFree(context); //释放连接对象
	    return false;
	}

    //如果认证成功，将连接加入连接池
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            redisFree(context); //释放连接对象
            return false;
        }
        connections_.push(context); //将重新连接成功的连接对象加入连接池的连接队列, 使其可供其他线程使用
        ++connection_count_; //增加连接池中连接对象的数量
    }
	cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
	return true;
}

void RedisConPool::returnConnection(redisContext* context)
{
	if (!context)
	    return;
	bool requestCheck = false;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			redisFree(context);
			return;
		}
		if (context->err) {
			//不可复用的连接不回到队列: 先销账并登记补池请求, 通知放到锁外
			redisFree(context);
			--connection_count_;
			check_requested_ = true;
			requestCheck = true;
		} else {
			connections_.push(context);
			cond_.notify_one();
		}
	}
	if (requestCheck) check_cond_.notify_one();
}

void RedisConPool::Close()
{
	{
	    std::lock_guard<std::mutex> lock(mutex_);
	    b_stop_ = true;
	}
	cond_.notify_all(); //唤醒所有等待线程, 使其检查停止标志并退出
	check_cond_.notify_all(); //唤醒检查线程, 使其检查停止标志并退出
    //等待检查线程退出
	if (check_thread_.joinable())
	    check_thread_.join();
	std::lock_guard<std::mutex> lock(mutex_);
	while (!connections_.empty())
	{
	    //释放连接池中的所有连接
	    redisFree(connections_.front());
	    connections_.pop();
	}
	connection_count_ = 0; //重置连接池中连接对象的数量
}

RedisConPool::~RedisConPool()
{
	Close(); //调用 Close 方法, 关闭连接池, 释放资源
	std::lock_guard<std::mutex> lock(mutex_);
	while (!connections_.empty()) {
		redisFree(connections_.front()); //释放连接池中的所有连接
		connections_.pop();
	}
}
