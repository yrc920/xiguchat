#include "CServer.h"
#include "HttpConnection.h"

CServer::CServer(net::io_context& ioc, unsigned short& port)
	: _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _ioc(ioc), _socket(ioc) {
	//acceptor:监听服务器所有地址对应端口的连接
}

void CServer::Start() {
	auto self = shared_from_this();
	//异步接受连接,socket是用来复用
	_acceptor.async_accept(_socket, [self](beast::error_code ec) {
		try {
			//出错放弃这链接, 继续监听其他链接
			if (ec) {
				self->Start();
				return;
			}

			//创建新连接,并创建HttpConnection类管理这个连接
			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			self->Start();//继续监听
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
		});
}
