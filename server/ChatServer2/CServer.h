#pragma once
// 服务器类, 负责监听端口, 接受连接, 管理会话
//

#include <boost/asio.hpp>
#include "CSession.h"
#include <memory>
#include <map>
#include <mutex>

using boost::asio::ip::tcp;

class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	void ClearSession(std::string); //清除指定uuid的会话

private:
	//处理接受连接的回调函数
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
	void StartAccept(); //开始接受连接

	boost::asio::io_context& _io_context; //主线程的io_context, 负责accept
	short _port; //监听的端口
	tcp::acceptor _acceptor; //acceptor对象, 用于接受连接
	std::map<std::string, std::shared_ptr<CSession>> _sessions; //存储所有会话
	std::mutex _mutex; //保护_sessions的互斥锁
};

