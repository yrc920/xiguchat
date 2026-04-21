#include "LogicSystem.h"
#include "HttpConnection.h"

LogicSystem::LogicSystem() {
	//注册一个GET请求的URL和对应的处理函数
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req";
        });
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler)); //将URL和处理函数插入到GET请求的处理器映射中
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	//如果没有找到对应URL的处理函数, 返回false, 让网络层返回404错误
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
	_get_handlers[path](con); //调用对应URL的处理函数, 处理请求
    return true;
} 
