#pragma once
// Grpc客户端类, 用于通过RPC调用Grpc服务器获取聊天服务器信息
//

#include "const.h"
#include "Singleton.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include <condition_variable>

class StatusConPool; //连接池类, 用于管理Grpc连接(前置声明)

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusGrpcClient : public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>; //允许Singleton类访问StatusGrpcClient的私有成员
public:
	~StatusGrpcClient() {} //允许Singleton类访问StatusGrpcClient的析构函数
	GetChatServerRsp GetChatServer(int uid); //通过RPC调用Grpc服务器获取聊天服务器信息
	LoginRsp Login(int uid, std::string token); //通过RPC调用Grpc服务器进行登录验证, 获取登录结果

private:
	StatusGrpcClient();

	std::unique_ptr<StatusConPool> pool_; //连接池对象, 用于管理Grpc连接

};

class StatusConPool
{
public:
	StatusConPool(size_t poolSize, std::string host, std::string port);
	//获取连接池中的一个连接, 如果没有可用连接则等待
	std::unique_ptr<StatusService::Stub> getConnection();
	//将使用完的连接归还给连接池
	void returnConnection(std::unique_ptr<StatusService::Stub> context);

	void Close(); //标记连接池停止, 唤醒所有等待的线程, 使其退出
	~StatusConPool();

private:
	std::atomic<bool> b_stop_; //连接池是否停止, 用于控制连接池的生命周期
	size_t poolSize_; //连接池的大小
	std::string host_; //Grpc服务器的主机地址
	std::string port_; //Grpc服务器的端口号
	//连接池中的连接队列, 存储可用的StatusService::Stub对象(用于发起RPC调用, 相当于信使)
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::mutex mutex_; //保护连接队列的互斥锁, 确保多线程环境下对连接池的安全访问
	std::condition_variable cond_; //条件变量, 用于在连接池为空时等待可用连接
};
