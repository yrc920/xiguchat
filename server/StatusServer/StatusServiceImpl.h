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
struct ChatServer; //聊天服务器信息(前置声明)

class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override; //重写基类的虚函数, 获取连接数最少的聊天服务器信息, 返回给客户端
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override; //重写基类的虚函数, 客户端登录, 验证用户id和token, 返回登录结果给客户端

private:
	void insertToken(int uid, std::string token); //将用户id和token插入到用户token列表中
	ChatServer getChatServer(); //从服务器列表中返回连接数最少的服务器信息

	std::unordered_map<std::string, ChatServer> _servers; //服务器列表, key为服务器名称, value为服务器信息
	std::mutex _server_mtx; //服务器列表的互斥锁, 保护_servers的线程安全
	std::unordered_map<int, std::string> _tokens; //用户token列表, key为用户id, value为token
	std::mutex _token_mtx; //用户token列表的互斥锁, 保护_tokens的线程安全
};

struct ChatServer {
	std::string host; //IP地址
	std::string port; //端口号
	std::string name; //服务器名称
	int con_count; //连接数
};
