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

class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>; //使基类创建实例时可以访问私有构造函数
public:
	//通过RPC调用Grpc服务器获取验证码
	GetVerifyRsp GetVerifyCode(std::string email) {
		ClientContext context; //创建RPC调用的上下文对象
		GetVerifyRsp reply; //RPC调用结果的响应对象
		GetVerifyReq request; //RPC调用的请求对象
		request.set_email(email); //设置请求对象的email字段为传入的email参数(在.proto文件中定义的)

		//Grpc客户端发起RPC调用
		Status status = stub_->GetVerifyCode(&context, request, &reply);
		if (status.ok()) {
			return reply;
		}
		else {
			//如果RPC调用失败，设置响应对象的error字段为RPCFailed错误码
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}

private:
	VerifyGrpcClient() {
		//创建一个gRPC通道，使用不安全的凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051",
			grpc::InsecureChannelCredentials());
		stub_ = VerifyService::NewStub(channel); //使用通道创建一个VerifyService的stub对象
	}

	std::unique_ptr<VerifyService::Stub> stub_; //用于发起RPC调用, 相当于信使
};