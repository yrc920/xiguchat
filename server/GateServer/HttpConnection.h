#pragma once
// 用于管理每个连接, 处理请求, 发送响应
//

#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem; //使LogicSystem可以访问HttpConnection的私有成员, 以便在处理请求时设置响应内容
public:
	HttpConnection(tcp::socket socket);
	void Start(); //开始处理连接

private:
	void CheckDeadline(); //检查连接是否超时, 超时则关闭连接
	void WriteResponse(); //发送响应
	void HandleReq(); //处理请求
	void PreParseGetParam(); //预处理GET请求的URL, 将URL中的参数解析到_get_params中

	tcp::socket  _socket; //连接套接字, 用于与客户端通信
	beast::flat_buffer _buffer{ 8192 }; //用于执行读操作的缓冲区
	http::request<http::dynamic_body> _request; //请求消息
	http::response<http::dynamic_body> _response; //响应消息
	// 用于在连接处理上设置截止时间的定时器
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};
	std::string _get_url; //GET请求的URL路径, 不包含查询字符串
	std::unordered_map<std::string, std::string> _get_params; //GET请求的参数映射, 键为参数名, 值为参数值
};
