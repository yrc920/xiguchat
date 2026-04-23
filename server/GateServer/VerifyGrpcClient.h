#pragma once
// gRPC客户端类, 用于通过RPC调用Grpc服务器获取验证码
//

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

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

	std::unique_ptr<VerifyService::Stub> stub_; //用于发起RPC调用, 相当于信使
};