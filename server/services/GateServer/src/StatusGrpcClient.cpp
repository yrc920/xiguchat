#include "StatusGrpcClient.h"
#include "ConfigMgr.h"

StatusGrpcClient::StatusGrpcClient()
{
	//从配置管理器中获取Grpc服务器的主机地址和端口号
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	pool_.reset(new StatusConPool(5, host, port)); //初始化连接池对象
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context; //创建Grpc客户端上下文对象
	GetChatServerRsp reply; //RPC调用结果的响应对象
	GetChatServerReq request; //RPC调用的请求对象
	request.set_uid(uid); //设置请求对象的uid字段为传入的参数uid

	auto stub = pool_->getConnection(); //从连接池中获取一个Grpc连接对象(Stub)
	//使用Grpc连接对象发起RPC调用, 将请求对象和响应对象传入, 获取调用结果的状态
	Status status = stub->GetChatServer(&context, request, &reply);

	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub)); //将使用完的Grpc连接对象归还给连接池
		});

	if (status.ok()) {
		return reply;
	}
	else {
		//如果RPC调用失败，设置响应对象的error字段为RPCFailed错误码
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}


StatusConPool::StatusConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false)
{
	for (size_t i = 0; i < poolSize_; ++i)
	{
		//创建一个gRPC通道，使用不安全的凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		//使用通道创建一个StatusService的stub对象, 并将其放入连接池中
		connections_.push(StatusService::NewStub(channel));
	}
}

std::unique_ptr<StatusService::Stub> StatusConPool::getConnection()
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

	//从连接池中获取一个可用的连接对象(Stub)
	auto context = std::move(connections_.front());
	connections_.pop(); //从连接池中移除获取的连接对象
	return context;
}

void StatusConPool::returnConnection(std::unique_ptr<StatusService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不将连接对象放回连接池中
	if (b_stop_) {
		return;
	}

	connections_.push(std::move(context)); //将使用完的连接对象放回连接池中
	cond_.notify_one(); //通知一个等待连接的线程, 使其能够从连接池中获取到归还的连接对象
}

void StatusConPool::Close()
{
	b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	cond_.notify_all(); //通知所有等待连接的线程, 使其能够从等待状态中退出
}

StatusConPool::~StatusConPool()
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	Close(); //调用Close()方法标记连接池停止, 唤醒所有等待的线程, 使其退出
	//清空连接池中的连接对象, 释放资源
	while (!connections_.empty()) {
		connections_.pop();
	}
}
