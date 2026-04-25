#pragma once
// gRPC客户端类, 用于通过RPC调用Grpc服务器获取验证码
//

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"

class RPConPool; //连接池类, 用于管理Grpc连接(前置声明)

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>; //使基类创建实例时可以访问私有构造函数
public:
	~VerifyGrpcClient() {} //使基类析构时可以访问派生类的析构函数
	GetVerifyRsp GetVerifyCode(std::string email); //通过RPC调用Grpc服务器获取验证码

private:
	VerifyGrpcClient();

	std::unique_ptr<RPConPool> pool_; //连接池对象, 用于管理Grpc连接
};

class RPConPool {
public:
	RPConPool(size_t poolSize, std::string host, std::string port);
	//从连接池中获取一个可用的VerifyService::Stub对象, 如果连接池为空则等待直到有连接可用
	std::unique_ptr<VerifyService::Stub> getConnection(); 
	//将使用完的VerifyService::Stub对象返回连接池, 使其可供其他线程使用
	void returnConnection(std::unique_ptr<VerifyService::Stub> context); 

	void Close(); //标记连接池停止, 唤醒所有等待的线程, 使其退出
	~RPConPool();

private:
	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	size_t poolSize_; //连接池的大小
	std::string host_; //Grpc服务器的主机地址
	std::string port_; //Grpc服务器的端口号
	//连接池中的连接队列, 存储可用的VerifyService::Stub对象(用于发起RPC调用, 相当于信使)
	std::queue<std::unique_ptr<VerifyService::Stub>> connections_; 
	std::mutex mutex_; //用于保护连接队列的互斥锁
	std::condition_variable cond_; //用于在连接池为空时等待的条件变量
};
