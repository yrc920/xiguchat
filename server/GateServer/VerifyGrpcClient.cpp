#include "VerifyGrpcClient.h"

VerifyGrpcClient::VerifyGrpcClient() {
	//创建一个gRPC通道，使用不安全的凭据
	std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051",
		grpc::InsecureChannelCredentials());
	stub_ = VerifyService::NewStub(channel); //使用通道创建一个VerifyService的stub对象
}

GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
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
