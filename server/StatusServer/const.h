#pragma once
// 这个头文件包含了所有的公共头文件, 以及一些公共的定义
//

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>
#include <iostream>
#include <string>
#include <functional>
#include "Singleton.h"
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <atomic>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <hiredis/hiredis.h>
#include <cassert>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001, //Json解析错误
	RPCFailed = 1002, //RPC请求错误
	VerifyExpired = 1003, //验证码过期
	VerifyCodeErr = 1004, //验证码错误
	UserExist = 1005, //用户已经存在
	PasswdErr = 1006, //密码错误
	EmailNotMatch = 1007, //邮箱不匹配
	PasswdUpFailed = 1008, //更新密码失败
	PasswdInvalid = 1009, //密码更新失败
	TokenInvalid = 1010,   //Token失效
	UidInvalid = 1011,  //uid无效
};

//Defer类(go语言风格的延迟执行)
class Defer {
public:
	//接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	//析构时执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};

#define CODEPREFIX "code_"
