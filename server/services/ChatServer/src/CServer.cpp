#include "CServer.h"
#include <iostream>
#include "IOContextPool.h"
#include "UserMgr.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"

CServer::CServer(boost::asio::io_context& io_context, short port) : _io_context(io_context),
    _port(port), _acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
    _timer(_io_context, std::chrono::seconds(HEART_BEAT_INTERVAL))
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
		_sessions.insert(std::make_pair(new_session->GetSessionId(), new_session));
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

void CServer::ClearSession(std::string session_id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.find(session_id) != _sessions.end()) {
        auto uid = _sessions[session_id]->GetUserId();

        //移除用户和session的关联
        UserMgr::GetInstance()->RmvUserSession(uid, session_id);
    }

    _sessions.erase(session_id);
}

bool CServer::CheckValid(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex); //保护_sessions的互斥锁
    auto it = _sessions.find(uuid); //查找指定uuid的会话
    //如果找到了, 说明会话存在, 返回true; 否则返回false
    if (it != _sessions.end())
        return true;

    return false;
}

std::shared_ptr<CSession> CServer::GetSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end()) {
        return it->second;
    }
    return nullptr;
}

void CServer::on_timer(const boost::system::error_code& ec) {
    if (ec) {
        std::cout << "timer error: " << ec.message() << std::endl;
        return;
    }
    std::vector<std::shared_ptr<CSession>> _expired_sessions;
    int session_count = 0;
    //此处加锁遍历session
    std::map<std::string, std::shared_ptr<CSession>> sessions_copy;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        sessions_copy = _sessions;
    }

    time_t now = std::time(nullptr);
    for (auto iter = sessions_copy.begin(); iter != sessions_copy.end(); iter++) {
        auto b_expired = iter->second->IsHeartbeatExpired(now);
        if (b_expired) {
            //关闭socket, 其实这里也会触发async_read的错误处理
            iter->second->Close();
            //收集过期信息
            _expired_sessions.push_back(iter->second);
            continue;
        }
        session_count++;
    }

    //设置session数量
    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    auto count_str = std::to_string(session_count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

    //处理过期session, 单独提出，防止死锁
    for (auto &session : _expired_sessions) {
        session->DealExceptionSession();
    }

    //再次设置，下一个60s检测
    _timer.expires_after(std::chrono::seconds(HEART_BEAT_INTERVAL));
    _timer.async_wait([this](boost::system::error_code ec) {
        on_timer(ec);
    });
}

void CServer::StartTimer()
{
    //启动定时器
    auto self(shared_from_this());
    _timer.async_wait([self](boost::system::error_code ec) {
        self->on_timer(ec);
        });
}

void CServer::StopTimer()
{
    _timer.cancel();
}
