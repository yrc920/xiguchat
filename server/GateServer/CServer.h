#pragma once
// 监听一个端口, 创建acceptor, 接受所有过来连接的socket
//

#include "const.h"

class CServer : public std::enable_shared_from_this<CServer>
{
public:
	CServer(net::io_context& ioc, unsigned short& port);
	void Start(); //开始监听连接

private:
	tcp::acceptor _acceptor; //监听器, 用于监听端口, 接受连接
	net::io_context& _ioc; //用于执行异步操作
};

