#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"

#include "CSession.h"
#include "MysqlMgr.h"

ChatGrpcClient::ChatGrpcClient()
{
	//从配置文件中读取服务器列表
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["PeerServer"]["Servers"];

	std::vector<std::string> words; //存储服务器列表中的单个服务器地址
	std::stringstream ss(server_list); //使用字符串流将服务器列表字符串分割成单个服务器地址
	std::string word; //临时变量用于存储分割出的单个服务器地址

	//将服务器列表字符串按照逗号分隔成单个服务器地址，并存储在words向量中
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		//如果配置文件中没有该服务器的Name字段，则跳过该服务器地址
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		//为每个服务器地址创建一个ChatConPool对象，并将其存储在_pools映射表中
		_pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5,
			cfg[word]["Host"], cfg[word]["Port"]);
	}
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp; //创建AddFriendRsp消息对象，准备存储RPC调用的结果
	Defer defer([&rsp, &req]() {
		rsp.set_error(ErrorCodes::Success); //默认返回成功状态码
		rsp.set_applyuid(req.applyuid()); //设置申请人UID
		rsp.set_touid(req.touid()); //设置目标用户UID
		});

	auto find_iter = _pools.find(server_ip); //在连接池映射表中查找与服务器IP地址对应的连接池
	//如果没有找到对应的连接池，则返回默认的AddFriendRsp对象
	if (find_iter == _pools.end())
		return rsp;

	auto& pool = find_iter->second; //获取对应服务器的连接池
	ClientContext context; //创建一个ClientContext对象，用于存储RPC调用的上下文信息
	auto stub = pool->getConnection(); //从连接池中获取一个ChatService::Stub对象，用于发起RPC调用
	//使用获取的Stub对象调用NotifyAddFriend方法，传入上下文、请求消息和响应消息对象，并获取调用状态
	Status status = stub->NotifyAddFriend(&context, req, &rsp);

	//使用Defer对象确保在函数结束时将Stub对象归还给连接池
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	//如果RPC调用失败
	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed); //设置错误状态码
		return rsp;
	}
	return rsp; //返回RPC调用的结果
}


bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
	const TextChatMsgReq& req, const Json::Value& rtvalue) {

	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}

		});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}


ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false)
{
	for (size_t i = 0; i < poolSize_; ++i)
	{
		//创建一个gRPC通道，使用不安全的凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		//创建一个ChatService的Stub对象，并将其添加到连接池中
		connections_.push(ChatService::NewStub(channel));
	}
}

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//等待条件变量, 直到连接池中有可用连接或者连接池被标记为停止
	cond_.wait(lock, [this] {
		//如果连接池被标记为停止, 则直接返回true, 保持锁定状态并继续执行
		if (b_stop_) {
			return true;
		}
		//如果没有可用连接, 返回false, 解锁并进入等待状态
		//如果有可用连接, 返回true, 保持锁定状态并继续执行
		return !connections_.empty();
		});
	//如果停止则直接返回空指针
	if (b_stop_) {
		return  nullptr;
	}

	//从连接池中获取一个可用连接对象(Stub)
	auto context = std::move(connections_.front());
	connections_.pop(); //从连接池中移除获取的连接对象
	return context;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不将连接对象放回连接池中
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context)); //将连接对象放回连接池中
	cond_.notify_one(); //通知一个等待的线程, 让它能够及时获取到放回连接池中的连接对象
}

void ChatConPool::Close()
{
	b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	cond_.notify_all(); //通知所有等待的线程, 让它们能够及时退出等待状态
}

ChatConPool::~ChatConPool()
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	Close(); //调用Close()方法标记连接池停止, 唤醒所有等待的线程, 使其退出
	//清空连接池中的连接对象, 释放资源
	while (!connections_.empty()) {
		connections_.pop();
	}
}
