#include "HttpConnection.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket)
	: _socket(std::move(socket)) {} //将socket移动构造到HttpConnection类中, 转移控制权

void HttpConnection::Start() {
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

//将十进制的char转为16进制
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

//将16进制转为十进制的char
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

//URL编码函数, 将字符串中的特殊字符转换为%加两位十六进制数的形式
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

//URL解码函数, 将%加两位十六进制数的形式转换回原来的字符, 同时将+转换为空格
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::PreParseGetParam() {
	//提取 URI  
	auto uri = _request.target();
	//查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	//如果没有查询字符串，直接返回
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos); //提取 URL 路径部分, 不包含查询字符串
	std::string query_string = uri.substr(query_pos + 1); //提取查询字符串部分
	std::string key; 
	std::string value;
	size_t pos = 0;
	//循环处理查询字符串中的每个参数对, 以 '&' 分隔
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos); //提取一个参数对
		size_t eq_pos = pair.find('='); //查找参数对中 '=' 的位置, 用于分隔键和值
		//如果找到了 '=', 则将 '=' 前的部分作为键, '=' 后的部分作为值, 并进行URL解码后存储到_get_params中
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos));
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1); //从查询字符串中删除已处理的参数对, 继续处理下一个参数对
	}
	//处理最后一个参数对（如果没有 & 分隔符）
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');	//查找参数对中 '=' 的位置, 用于分隔键和值
		//如果找到了 '=', 则将 '=' 前的部分作为键, '=' 后的部分作为值, 并进行URL解码后存储到_get_params中
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

void HttpConnection::HandleReq() {
	//设置响应的HTTP版本和连接选项
	_response.version(_request.version());
	_response.keep_alive(false);

	//如果是GET方法
	if (_request.method() == http::verb::get) {
		PreParseGetParam(); //预处理GET请求的URL, 将URL中的参数解析到_get_params中
		//调用LogicSystem处理GET请求, 传入纯路径URL
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
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

	//如果是POST方法
	if (_request.method() == http::verb::post) {
		//调用LogicSystem处理POST请求, 传入URL
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
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
