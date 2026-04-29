#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"

LogicSystem::LogicSystem() {
	//注册一个GET请求的URL和对应的处理函数
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req" << std::endl;

		//如果URL中有参数, 则将参数键值对写入响应内容中, 以便测试GET请求的参数解析功能
		int i = 0; //用于计数参数的序号, 从1开始
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", " << " value is " << elem.second << std::endl;
		}
    });

	//注册发送验证码的POST请求的URL和对应的处理函数
	RegPost("/get_verifycode", [](std::shared_ptr<HttpConnection> connection) {
		//将请求体中的数据转换为字符串
		auto body_str = beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json"); //设置响应内容类型为JSON

		Json::Value root; //用于构建响应的JSON数据
		Json::Reader reader; //用于解析请求体中的JSON数据
		Json::Value src_root; //用于存储解析后的JSON数据
		//解析请求体中的JSON数据, 将解析结果存储到src_root中 
		bool parse_success = reader.parse(body_str, src_root);
		//如果解析失败或者请求体中没有email字段, 则返回错误信息
		if (!parse_success || !src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json; //设置错误码为JSON解析错误
			//将JSON数据转换为字符串, 并写入响应内容中
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//如果解析成功, 则从src_root中提取email字段的值
		auto email = src_root["email"].asString();
		//给验证服务发送RPC请求, 获取验证码, 将RPC调用的结果存储在rsp中
		GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error(); //将RPC调用结果中的error字段设置到响应的JSON数据中
		root["email"] = src_root["email"]; //将请求中的email字段原样返回到响应中
		//将JSON数据转换为字符串, 并写入响应内容中
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	//注册用户注册的POST请求的URL和对应的处理函数
    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		//将请求体中的数据转换为字符串
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json"); //设置响应内容类型为JSON

		Json::Value root; //用于构建响应的JSON数据
		Json::Reader reader; //用于解析请求体中的JSON数据
		Json::Value src_root; //用于存储解析后的JSON数据
		//解析请求体中的JSON数据, 将解析结果存储到src_root中
        bool parse_success = reader.parse(body_str, src_root);
		//如果解析失败或者请求体中没有email、user、passwd、confirm和verifycode字段, 则返回错误信息
        if (!parse_success || 
			!src_root.isMember("email") || !src_root.isMember("user") || 
			!src_root.isMember("passwd") || !src_root.isMember("confirm") || 
			!src_root.isMember("verifycode")) {
            std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json; //设置错误码为JSON解析错误
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

		//如果解析成功, 则从src_root中提取email、user、passwd、confirm和verifycode字段的值
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		//检查密码和确认密码是否一致, 如果不一致则返回错误信息
		if (pwd != confirm) {
			std::cout << " confirm password error" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			//将JSON数据转换为字符串, 并写入响应内容中
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

        //先查找redis中email对应的验证码是否合理
        std::string  verify_code;
        bool b_get_verify = RedisMgr::GetInstance()->
            Get(CODEPREFIX + email, verify_code);
		//如果没有找到验证码或者验证码过期了, 则返回错误信息
        if (!b_get_verify) {
            std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired; //设置错误码为验证码过期
            //将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }
		//如果找到了验证码, 则将请求中的验证码和redis中获取到的验证码进行比较, 如果不一致则返回错误信息
        if (verify_code != src_root["verifycode"].asString()) {
            std::cout << " verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr; //设置错误码为验证码错误
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

		//查找数据库判断用户是否存在
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
		//如果uid为0或者-1, 则说明用户已经存在, 返回错误信息
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist; //设置错误码为用户已经存在
			//将JSON数据转换为字符串, 并写入响应内容中
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//如果用户不存在, 则将用户信息写入数据库, 返回注册成功的响应
        root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
        root["confirm"] = confirm;
        root["verifycode"] = src_root["verifycode"].asString();
		//将JSON数据转换为字符串, 并写入响应内容中
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
    });

	//注册用户重置密码的POST请求的URL和对应的处理函数
    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        //将请求体中的数据转化为字符串
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json"); //设置响应内容类型为JSON

		Json::Value root; //用于构建响应的JSON数据
		Json::Reader reader; //用于解析请求体中的JSON数据
		Json::Value src_root; //用于存储解析后的JSON数据
		//解析请求体中的JSON数据, 将解析结果存储到src_root中
        bool parse_success = reader.parse(body_str, src_root);
		//如果解析失败或者请求体中没有email、user、passwd和verifycode字段, 则返回错误信息
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json; //设置错误码为JSON解析错误
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

		//如果解析成功, 则从src_root中提取email、user、passwd和verifycode字段的值
        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();

        //先查找redis中email对应的验证码是否合理
        std::string verify_code;
        bool b_get_verify = RedisMgr::GetInstance()->
			Get(CODEPREFIX + src_root["email"].asString(), verify_code);
		//如果没有找到验证码或者验证码过期了, 则返回错误信息
        if (!b_get_verify) {
            std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired; //设置错误码为验证码过期
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }
		//如果找到了验证码, 则将请求中的验证码和redis中获取到的验证码进行比较, 如果不一致则返回错误信息
        if (verify_code != src_root["verifycode"].asString()) {
            std::cout << " verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr; //设置错误码为验证码错误
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        //查询数据库判断用户名和邮箱是否匹配
        bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		//如果用户名和邮箱不匹配, 则返回错误信息
        if (!email_valid) {
            std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch; //设置错误码为用户名和邮箱不匹配
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        //更新密码为最新密码
        bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		//如果更新密码失败, 则返回错误信息
        if (!b_up) {
            std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed; //设置错误码为更新密码失败
			//将JSON数据转换为字符串, 并写入响应内容中
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

		//如果更新密码成功, 则返回更新密码成功的响应
        std::cout << "succeed to update password" << pwd << std::endl;
        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["verifycode"] = src_root["verifycode"].asString();
		//将JSON数据转换为字符串, 并写入响应内容中
		std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
        });
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler)); //将URL和处理函数插入到GET请求的处理器映射中
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler)); //将URL和处理函数插入到POST请求的处理器映射中
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	//如果没有找到对应URL的处理函数, 返回false, 让网络层返回404错误
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
	_get_handlers[path](con); //调用对应URL的处理函数, 处理请求
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	//如果没有找到对应URL的处理函数, 返回false, 让网络层返回404错误
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}

	_post_handlers[path](con); //调用对应URL的处理函数, 处理请求
	return true;
}
