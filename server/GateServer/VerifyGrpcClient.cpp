#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

VerifyGrpcClient::VerifyGrpcClient()
{
	//从配置管理器中获取Grpc服务器的主机地址和端口号
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VerifyServer"]["Host"];
	std::string port = gCfgMgr["VerifyServer"]["Port"];
	pool_.reset(new RPConPool(5, host, port)); //初始化连接池对象
}

GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
	ClientContext context; //创建RPC调用的上下文对象
	GetVerifyRsp reply; //RPC调用结果的响应对象
	GetVerifyReq request; //RPC调用的请求对象
	request.set_email(email); //设置请求对象的email字段为传入的email参数(在.proto文件中定义的)

	auto stub = pool_->getConnection(); //从连接池中获取一个可用的VerifyService::Stub对象
	//Grpc客户端发起RPC调用
	Status status = stub->GetVerifyCode(&context, request, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub)); //将使用完的Stub对象归还给连接池
		return reply;
	}
	else {
		pool_->returnConnection(std::move(stub)); //将使用完的Stub对象归还给连接池
		//如果RPC调用失败，设置响应对象的error字段为RPCFailed错误码
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}


RPConPool::RPConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {
		//创建一个gRPC通道，使用不安全的凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		//使用通道创建一个VerifyService的stub对象, 并将其放入连接池中
		connections_.push(VerifyService::NewStub(channel));
	}
}

std::unique_ptr<VerifyService::Stub> RPConPool::getConnection()
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
	
	//从连接池中获取一个可用的VerifyService::Stub对象
	auto context = std::move(connections_.front());
	connections_.pop(); //从连接池中移除获取的Stub对象
	return context;
}

void RPConPool::returnConnection(std::unique_ptr<VerifyService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不再归还连接
	if (b_stop_)
		return;

	connections_.push(std::move(context)); //将Stub对象归还给连接池
	cond_.notify_one(); //通知一个等待连接的线程, 使其能够从连接池中获取到归还的连接对象
}

void RPConPool::Close()
{
	b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	cond_.notify_all(); //通知所有等待连接的线程, 使它们能够退出等待状态并检查停止标志
}

RPConPool::~RPConPool()
{
	std::lock_guard<std::mutex> lock(mutex_); //加锁保护连接池的状态
	Close(); //调用Close()方法标记连接池停止, 唤醒所有等待的线程, 使其退出
	//清空连接池中的连接对象, 释放资源
	while (!connections_.empty()) {
		connections_.pop();
	}
}
