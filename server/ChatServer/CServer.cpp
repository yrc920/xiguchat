#include "CServer.h"
#include "HttpConnection.h"
#include "IOContextPool.h"

CServer::CServer(net::io_context& ioc, unsigned short& port)
	: _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _ioc(ioc) {
	//acceptor:监听服务器所有地址对应端口的连接
}

void CServer::Start() {
	auto self = shared_from_this();
	//从IOContextPool中获取一个io_context对象
	auto& io_context = IOContextPool::GetInstance()->GetIOContext();
	//创建一个新的HttpConnection对象, 用于监听和管理
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);

	//异步接受连接, 先用HttpConnection的socket来监听连接, 
	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		try {
			//出错放弃这链接, 继续监听其他链接
			if (ec) {
				self->Start();
				return;
			}

			//成功接受连接后, 由HttpConnection对象来管理这个连接, 开始处理连接
			new_con->Start();

			self->Start(); //继续监听
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
	});
}
