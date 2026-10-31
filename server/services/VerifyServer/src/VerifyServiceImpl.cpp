#include "VerifyServiceImpl.h"
#include "EmailSender.h"
#include "RedisMgr.h"
#include "const.h"

#include <boost/uuid/uuid_io.hpp>

//context 由 gRPC 传入（本服务不使用），保留参数以符合基类签名
grpc::Status VerifyServiceImpl::GetVerifyCode(grpc::ServerContext* /*context*/,
	const message::GetVerifyReq* request,
	message::GetVerifyRsp* response)
{
	//无论成败都把邮箱回填给调用方，错误码单独设置
	auto email = request->email();
	response->set_email(email);

	try {
		std::string key = CODE_PREFIX + email; //Redis 键：code_<email>
		std::string code;

		//该邮箱在有效期内已申请过验证码时直接复用，避免短时间内重复发送不同验证码
		if (!RedisMgr::GetInstance()->Get(key, code)) {
			code = GenerateCode();
			//写入失败（Redis 不可用）时无法保证后续校验，直接返回 Redis 错误
			if (!RedisMgr::GetInstance()->SetExpire(key, code, CODE_EXPIRE_SECONDS)) {
				response->set_error(VerifyCodes::RedisErr);
				return grpc::Status::OK;
			}
		}

		std::string text = "您的验证码为 " + code + "，请在 " +
            std::to_string(CODE_EXPIRE_SECONDS / 60) + " 分钟内完成验证。";
		if (!EmailSender::Send(email, "验证码", text)) {
			response->set_error(VerifyCodes::SendMailErr);
			return grpc::Status::OK;
		}

		response->set_error(VerifyCodes::Success);
	}
	catch (std::exception& e) {
		std::cerr << "GetVerifyCode exception: " << e.what() << std::endl;
		response->set_error(VerifyCodes::Exception);
	}

	return grpc::Status::OK;
}

std::string VerifyServiceImpl::GenerateCode()
{
	//random_generator 非线程安全，用 thread_local 保证每个调用线程一份实例
	thread_local boost::uuids::random_generator generator;
	auto uuid_str = boost::uuids::to_string(generator());
	//UUID 形如 xxxxxxxx-xxxx-...，取前 4 位作为验证码（必定是十六进制字符）
	return uuid_str.substr(0, CODE_LENGTH);
}
