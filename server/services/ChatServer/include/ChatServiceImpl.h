#pragma once
// Grpc服务器实现类，负责处理客户端请求
//

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <mutex>
#include "data.h"
#include "CServer.h"
#include <memory>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;
using message::KickUserReq;
using message::KickUserRsp;

class ChatServiceImpl final : public ChatService::Service
{
public:
	ChatServiceImpl();
	Status NotifyAddFriend(ServerContext* context,
		const AddFriendReq* request, AddFriendRsp* reply) override; //处理添加好友请求
	Status NotifyAuthFriend(ServerContext* context,
		const AuthFriendReq* request, AuthFriendRsp* response) override; //处理好友验证请求
	Status NotifyTextChatMsg(ServerContext* context,
		const TextChatMsgReq* request, TextChatMsgRsp* response) override; //处理文本聊天消息请求
    Status NotifyKickUser(::grpc::ServerContext* context,
        const KickUserReq* request, KickUserRsp* response) override; //接受rpc踢人请求

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo); //获取用户基本信息
    void RegisterServer(std::shared_ptr<CServer> pServer); //注册CServer对象，方便在处理请求时访问服务器相关功能

private:
    std::shared_ptr<CServer> _p_server; //CServer对象的智能指针，用于在处理请求时访问服务器相关功能
};

