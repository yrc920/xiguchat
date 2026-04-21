#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket)
	: _socket(std::move(socket)) {} //将socket移动构造到HttpConnection类中, 转移控制权

void HttpConnection::Start()
{
	auto self = shared_from_this();
	//异步读取请求
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			try {
				if (ec) {
					std::cout << "http read err is " << ec.what() << std::endl;
					return;
				}
				//处理读到的数据
				boost::ignore_unused(bytes_transferred); //已读数据长度直接忽略
				self->HandleReq(); //处理请求
				self->CheckDeadline(); //同时启动连接超时检查
			}
			catch (std::exception& exp) {
				std::cout << "exception is " << exp.what() << std::endl;
			}
		});
}

void HttpConnection::HandleReq() {
	//设置响应的HTTP版本和连接选项
	_response.version(_request.version());
	_response.keep_alive(false);

	//如果是GET方法
	if (_request.method() == http::verb::get) {
		//调用LogicSystem处理GET请求
		bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
		//如果LogicSystem没有处理这个URL, 则返回404错误
		if (!success) {
			_response.result(http::status::not_found); //设置响应状态码为404 Not Found
			_response.set(http::field::content_type, "text/plain"); //设置响应内容类型为纯文本
			beast::ostream(_response.body()) << "url not found\r\n"; //设置响应内容为错误信息
			WriteResponse(); //发送响应
			return;
		}
		_response.result(http::status::ok); //设置响应状态码为200 OK
		_response.set(http::field::server, "GateServer"); //设置响应头中的服务器信息
		WriteResponse(); //发送响应
		return;
	}
}

void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size()); //设置响应内容长度
	//异步发送响应
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec); //发送完成后关闭服务器端
		self->deadline_.cancel(); //取消超时检查
		});
}

void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	//异步等待截止时间到达
	deadline_.async_wait([self](beast::error_code ec) {
		if (!ec) {
			//关闭套接字以取消任何未完成的操作
			self->_socket.close(ec);
		}
		});
}
