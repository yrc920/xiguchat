#include "ChatGrpcClient.h"
#include "RedisMgr.h"

ChatGrpcClient::ChatGrpcClient()
{
	//从配置管理器中获取服务器列表
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words; //存储服务器列表中的每个服务器名称
	std::stringstream ss(server_list); //使用字符串流将服务器列表字符串分割成单个服务器名称
	std::string word; //临时变量用于存储分割出的服务器名称

	//将服务器列表字符串按照逗号分割成单个服务器名称, 并将其存储到words向量中
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		//如果服务器名称为空, 则跳过该服务器
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		//为每个服务器名称创建一个ChatConPool对象, 并将其存储到连接池映射表中
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5,
			cfg[word]["Host"], cfg[word]["Port"]);
	}
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);

	AddFriendRsp rsp;
	return rsp;
}

ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false)
{
	for (size_t i = 0; i < poolSize_; ++i)
	{
		//创建一个grpc通道, 使用不安全的凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		//创建一个ChatService::Stub对象, 用于发起RPC调用, 将其加入连接池的连接队列
		connections_.push(ChatService::NewStub(channel));
	}
}

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//等待条件变量, 直到连接池中有可用的连接或者连接池被标记为停止
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

	//从连接池的连接队列中获取一个可用的连接对象(ChatService::Stub)
	auto context = std::move(connections_.front());
	connections_.pop(); //从连接池中移除获取的连接对象
	return context;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不再将连接对象放回池中
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context)); //将连接对象放回连接池的连接队列中
	cond_.notify_one(); //通知一个等待线程, 连接池中有可用的连接对象
}

void ChatConPool::Close()
{
	b_stop_ = true; //标记连接池停止, 使所有等待线程能够退出等待状态
	cond_.notify_all(); //通知所有等待线程, 连接池已停止, 使它们能够退出等待状态
}

ChatConPool::~ChatConPool()
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	Close(); //调用Close方法, 标记连接池停止, 使所有等待线程能够退出等待状态
	//清空连接池的连接队列, 释放所有连接对象的资源
	while (!connections_.empty()) {
		connections_.pop();
	}
}
