#pragma once
// Grpc服务的实现类, 负责处理客户端的请求, 返回相应的结果
//

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <unordered_map>
#include <mutex>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;
//ChatServer作为unordered_map的value类型，需要先给出完整定义再声明容器成员
//只有前置声明不足以提供元素布局和析构所需的信息，因此结构定义放在服务类之前
struct ChatServer {
	std::string host; //IP地址
	std::string port; //返回给客户端连接的TCP端口，不是服务间RPC端口
	std::string name; //服务器名称
	int con_count = 0; //连接数
};

class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();
	//供网关获取候选聊天实例并签发token，业务失败通过reply中的error返回
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	//校验uid和token的RPC接口；ChatServer当前直接读Redis校验，并未调用这个接口
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;

private:
	bool insertToken(int uid, std::string token); //写入用户token，并返回Redis写入是否成功
	ChatServer getChatServer(); //选择连接数最小的有效候选，没有候选时返回name为空的结构

	std::unordered_map<std::string, ChatServer> _servers; //服务器列表, key为服务器名称, value为服务器信息
	std::mutex _server_mtx; //服务器列表的互斥锁, 保护_servers的线程安全

};
