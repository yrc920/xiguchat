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
	: b_stop_(false) {
	//创建 poolSize 条连接；连接参数只在建立时使用，之后由队列管理
	for (size_t i = 0; i < poolSize; ++i) {
		auto* context = redisConnect(host, port); //连接 Redis 服务器
		//连接失败则释放该连接并继续尝试下一个
		if (context == nullptr || context->err != 0) {
			if (context != nullptr) {
				redisFree(context);
			}
			continue;
		}

		//执行 AUTH 命令进行密码认证
		auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cout << "Authentication failure" << std::endl;
			freeReplyObject(reply);
			redisFree(context);
			continue;
		}

		freeReplyObject(reply);
		std::cout << "Authentication successful" << std::endl;
		connections_.push(context); //将连接加入队列, 供其他线程使用
	}
}

redisContext* RedisConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池状态
	//等待有可用连接或连接池被标记为停止
	cond_.wait(lock, [this] {
		if (b_stop_) {
			return true;
		}
		return !connections_.empty();
	});
	if (b_stop_) {
		return nullptr;
	}
	auto* context = connections_.front();
	connections_.pop();
	return context;
}

void RedisConPool::returnConnection(redisContext* context)
{
	std::lock_guard<std::mutex> lock(mutex_);
	//连接池已停止时不再归还, 避免等待线程拿到已废弃的连接
	if (b_stop_) {
		return;
	}
	connections_.push(context);
	cond_.notify_one(); //通知一个等待连接的线程
}

void RedisConPool::Close()
{
	b_stop_ = true; //设置停止标志
	cond_.notify_all(); //唤醒所有等待线程, 使其检查停止标志并退出
}

RedisConPool::~RedisConPool()
{
	std::lock_guard<std::mutex> lock(mutex_);
	while (!connections_.empty()) {
		redisFree(connections_.front()); //释放连接池中的所有连接
		connections_.pop();
	}
}
