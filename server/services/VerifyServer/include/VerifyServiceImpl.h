#pragma once
// gRPC 服务实现：VerifyService::GetVerifyCode
//
// 处理流程：查 Redis 中该邮箱是否已有验证码 → 有则复用，无则生成并写入 → 发送邮件

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

class VerifyServiceImpl final : public message::VerifyService::Service
{
public:
	// RPC 入口：request 携带邮箱，response 回填错误码与邮箱
	grpc::Status GetVerifyCode(grpc::ServerContext* context,
		const message::GetVerifyReq* request,
		message::GetVerifyRsp* response) override;

private:
	//生成验证码（UUID 前若干位），保证同一邮箱有效期内复用同一个验证码
	std::string GenerateCode();
};
