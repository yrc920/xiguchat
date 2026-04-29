#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::RedisMgr()
{
	//从配置管理器中获取Redis服务器的主机地址、端口号和密码
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	//初始化连接池对象, 连接池大小为5
	_con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 获取key对应的value
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());

	//如果返回NULL则说明执行失败, 释放连接并返回false
	if (reply == NULL) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}
	//如果返回的类型不是字符串, 则说明执行失败, 释放连接并返回false
	if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	value = reply->str; //将获取到的value赋值给传入的value参数
	std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
	freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 设置key和value
	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());

	//如果返回NULL则说明执行失败, 释放连接并返回false
	if (NULL == reply)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}
	//如果返回的类型不是状态或者状态不是OK, 则说明执行失败, 释放连接并返回false
	if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 将value插入到key对应的列表的头部
	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());

	//如果返回NULL则说明执行失败, 释放连接并返回false
	if (NULL == reply)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}
	//如果返回的类型不是整数或者整数值小于等于0(成功将返回1), 则说明执行失败, 释放连接并返回false
	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 移除并返回key对应的列表的头部元素
	auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());

	//如果返回NULL或者返回的类型是REDIS_REPLY_NIL(表示列表为空), 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	value = reply->str; //将获取到的元素赋值给传入的value参数
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 将value插入到key对应的列表的尾部
	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());

	//如果返回NULL则说明执行失败, 释放连接并返回false
	if (NULL == reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}
	//如果返回的类型不是整数或者整数值小于等于0(成功将返回1), 则说明执行失败, 释放连接并返回false
	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 移除并返回key对应的列表的尾部元素
	auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());

	//如果返回NULL或者返回的类型是REDIS_REPLY_NIL(表示列表为空), 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	value = reply->str; //将获取到的元素赋值给传入的value参数
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 设置哈希表key中字段hkey的值为value
	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());

	//如果返回NULL或者返回的类型不是整数, 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}
	
	const char* argv[4]; //存储redis命令行的参数数组, 包含命令和参数
	size_t argvlen[4]; //存储redis命令行的参数长度数组, 与argv数组对应, 用于支持二进制数据
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;

	//执行redis命令行, 设置哈希表key中字段hkey的值为hvalue, 支持二进制数据
	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);

	//如果返回NULL或者返回的类型不是整数, 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回空字符串
	if (connect == nullptr) {
		return "";
	}

	const char* argv[3]; //存储redis命令行的参数数组, 包含命令和参数
	size_t argvlen[3]; //存储redis命令行的参数长度数组, 与argv数组对应, 用于支持二进制数据
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	
	//执行redis命令行, 获取哈希表key中字段hkey的值
	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);

	//如果返回NULL或者返回的类型是REDIS_REPLY_NIL(表示字段不存在), 则说明执行失败, 释放连接并返回空字符串
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return "";
	}

	std::string value = reply->str; //将获取到的值赋值给value变量
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return value;
}

bool RedisMgr::Del(const std::string& key)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 删除key
	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());

	//如果返回NULL或者返回的类型不是整数, 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_con_pool->returnConnection(connect);
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	auto connect = _con_pool->getConnection(); //从连接池中获取一个可用的redisContext对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (connect == nullptr) {
		return false;
	}

	//执行redis命令行, 检查key是否存在
	auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());

	//如果返回NULL或者返回的类型不是整数, 则说明执行失败, 释放连接并返回false
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
		_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
		return false;
	}

	//执行成功, 释放redisCommand执行后返回的redisReply所占用的内存
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply);
	_con_pool->returnConnection(connect); //将使用完的redisContext对象归还给连接池
	return true;
}

void RedisMgr::Close()
{
	_con_pool->Close(); //调用连接池的Close方法, 标记连接池停止, 唤醒所有等待的线程, 使其退出
}

RedisMgr::~RedisMgr()
{
	Close(); //调用Close方法, 关闭连接池, 释放资源
}


RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {
		auto* context = redisConnect(host, port);//连接Redis服务器, 获取一个redisContext对象
		//如果连接对象为nullptr或者连接错误, 则说明连接失败, 释放连接对象并继续尝试连接下一个
		if (context == nullptr || context->err != 0) {
			//如果连接对象不为nullptr, 则说明连接对象存在但连接错误, 释放连接对象
			if (context != nullptr) {
				redisFree(context);
			}
			continue;
		}

		//执行AUTH命令进行密码认证
		auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
		//如果返回的类型是错误, 则说明认证失败, 释放连接对象并继续尝试连接下一个
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cout << "Authentication failure" << std::endl;
			freeReplyObject(reply); //释放redisCommand执行后返回的redisReply所占用的内存
			redisFree(context); //释放连接对象
			continue;
		}

		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(reply);
		std::cout << "Authentication successful" << std::endl;
		connections_.push(context); //将连接对象加入连接池的连接队列, 使其可供其他线程使用
	}

}

redisContext* RedisConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//使用条件变量等待连接池中有可用的连接或者连接池被标记为停止
	cond_.wait(lock, [this] {
		//如果连接池被标记为停止, 则直接返回true, 保持锁定状态并继续执行
		if (b_stop_) {
			return true;
		}
		//如果没有可用的连接, 返回false, 解锁并进入等待状态;
		//如果有可用的连接, 返回true, 保持锁定状态并继续执行
		return !connections_.empty();
		});
	//如果停止则直接返回空指针
	if (b_stop_) {
		return  nullptr;
	}
	//从连接池中获取一个可用的redisContext对象
	auto* context = connections_.front();
	connections_.pop(); //从连接池中移除获取的redisContext对象
	return context;
}

void RedisConPool::returnConnection(redisContext* context)
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不将连接对象加入连接池
	if (b_stop_) {
		return;
	}

	connections_.push(context); //将连接对象加入连接池的连接队列, 使其可供其他线程使用
	cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
}

void RedisConPool::Close()
{
	b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	cond_.notify_all(); //通知所有等待连接的线程, 使它们能够退出等待状态并检查停止标志
}

RedisConPool::~RedisConPool()
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//释放连接池中的所有连接对象, 直到连接队列为空
	while (!connections_.empty()) {
		redisFree(connections_.front()); //释放连接对象
		connections_.pop();
	}
}
