#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"

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

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		if (pwd != confirm) {
			std::cout << " confirm password error" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

        //先查找redis中email对应的验证码是否合理
        std::string  verify_code;
        bool b_get_verify = RedisMgr::GetInstance()->
            Get(CODEPREFIX + email, verify_code);
        if (!b_get_verify) {
            std::cout << " get verify code expired" << std::endl;
            root["error"] = ErrorCodes::VerifyExpired;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        if (verify_code != src_root["verifycode"].asString()) {
            std::cout << " verify code error" << std::endl;
            root["error"] = ErrorCodes::VerifyCodeErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        
        //查找数据库判断用户是否存在

        root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
        root["confirm"] = confirm;
        root["verifycode"] = src_root["verifycode"].asString();
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
