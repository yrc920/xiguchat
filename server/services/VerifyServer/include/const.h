#pragma once
// 公共头文件与全局定义（错误码、Redis 键前缀）
//

#include "Singleton.h"

#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <map>
#include <unordered_map>
#include <condition_variable>
#include <cstring>

#include <hiredis/hiredis.h>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// 验证码服务返回的错误码，由 GateServer 原样透传给客户端
enum VerifyCodes {
	Success = 0,      // 验证码已发送
	RedisErr = 1,     // 读写 Redis 失败
	Exception = 2,    // 处理过程异常
	SendMailErr = 3   // 发送邮件失败
};

// Redis 中验证码键的前缀，完整键为 code_<email>
#define CODE_PREFIX "code_"
// 验证码有效期（秒）
#define CODE_EXPIRE_SECONDS 600
// 验证码长度（取 UUID 前若干位）
#define CODE_LENGTH 4
