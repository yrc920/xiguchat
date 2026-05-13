#include "CServer.h"
#include <iostream>
#include "IOContextPool.h"

CServer::CServer(boost::asio::io_context& io_context, short port) : _io_context(io_context),
_port(port), _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
	std::cout << "Server start success, listen on port : " << _port << std::endl;
	StartAccept(); //开始接受连接
}

CServer::~CServer() {
	std::cout << "Server destruct listen on port : " << _port << std::endl;
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error)
{
	//如果没有错误, 则启动新的会话, 否则输出错误信息
	if (!error) {
		new_session->Start(); //启动会话, 开始处理连接
		std::lock_guard<std::mutex> lock(_mutex); //保护_sessions的互斥锁
		//将新的会话添加到_sessions中
		_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));
	}
	else {
		std::cout << "session accept failed, error is " << error.what() << std::endl;
	}

	StartAccept(); //继续接受下一个连接
}

void CServer::StartAccept()
{
	//从IOContext池中获取一个IOContext对象, 并创建一个新的会话对象来接受连接
	auto& io_context = IOContextPool::GetInstance()->GetIOContext();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);

	//异步接受连接, 当有新的连接到来时, 会调用HandleAccept函数
	_acceptor.async_accept(new_session->GetSocket(),
		std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1)); 
}

void CServer::ClearSession(std::string uuid)
{
	std::lock_guard<std::mutex> lock(_mutex); //保护_sessions的互斥锁
	_sessions.erase(uuid); //从_sessions中删除指定uuid的会话
}
