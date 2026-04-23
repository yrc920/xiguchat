#pragma once
// 逻辑系统, 用于处理HTTP请求, 根据URL调用对应的处理函数
//

#include "const.h"
class HttpConnection; //防止互引用
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler; //函数处理器

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>; //使基类创建实例时可以访问私有构造函数
public:
    ~LogicSystem() {} //使基类析构时可以访问派生类的析构函数
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>); //处理GET请求
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>); //处理POST请求
	void RegGet(std::string, HttpHandler handler); //注册GET请求的URL和对应的处理函数
	void RegPost(std::string, HttpHandler handler); //注册POST请求的URL和对应的处理函数

private:
    LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers; //处理POST请求的URL和对应的处理函数
	std::map<std::string, HttpHandler> _get_handlers; //处理GET请求的URL和对应的处理函数
};
