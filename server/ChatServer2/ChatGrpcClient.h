#pragma once
// Grpc客户端类, 负责与其他聊天服务器进行通信
//

#include "const.h"
#include "Singleton.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;
class ChatConPool; //连接池类, 用于管理Grpc连接(前置声明)

class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>; //允许Singleton类访问ChatGrpcClient的私有成员
public:
	~ChatGrpcClient() {} //允许Singleton类访问ChatGrpcClient的析构函数

	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip,
		const TextChatMsgReq& req, const Json::Value& rtvalue);

private:
	ChatGrpcClient();
	//连接池映射表, 用于管理与该服务器的Grpc连接
	std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};

class ChatConPool
{
public:
	ChatConPool(size_t poolSize, std::string host, std::string port);
	~ChatConPool();
	std::unique_ptr<ChatService::Stub> getConnection(); //获取连接, 如果没有可用连接则等待
	void returnConnection(std::unique_ptr<ChatService::Stub> context); //归还连接, 将连接放回连接池
	void Close(); //关闭连接池, 释放所有连接资源

private:
	std::atomic<bool> b_stop_; //连接池是否停止
	size_t poolSize_; //连接池大小
	std::string host_; //服务器IP地址
	std::string port_; //服务器端口
	//连接池中的连接队列, 存储可用的StatusService::Stub对象(用于发起RPC调用, 相当于信使)
	std::queue<std::unique_ptr<ChatService::Stub>> connections_;
	std::mutex mutex_; //保护连接队列的互斥锁
	std::condition_variable cond_; //条件变量, 用于等待连接池中有可用连接
};
