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
        //先把连接加入_sessions，再启动它的异步读取，否则读取回调可能先执行并被CheckValid判为无效
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _sessions.emplace(new_session->GetSessionId(), new_session);
        }
        new_session->Start();
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

        //按uid和session_id一起解绑，只有映射仍指向当前连接时才移除，避免误删同一用户的新连接
        UserMgr::GetInstance()->RmvUserSession(uid, session_id);
    }

    _sessions.erase(session_id); //只移除本地会话登记，socket关闭与Redis状态清理由调用方处理
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
    //持锁复制会话表后再遍历快照，避免检测心跳和访问Redis时长时间占用会话表的锁
    std::map<std::string, std::shared_ptr<CSession>> sessions_copy;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        sessions_copy = _sessions;
    }

    time_t now = std::time(nullptr);
    for (auto iter = sessions_copy.begin(); iter != sessions_copy.end(); iter++) {
        auto b_expired = iter->second->IsHeartbeatExpired(now);
        if (b_expired) {
            //标记关闭并投递socket关闭操作，未完成的读取也可能随后报错，清理入口需要能处理重复调用
            iter->second->Close();
            //收集过期信息
            _expired_sessions.push_back(iter->second);
            continue;
        }
        session_count++;
    }

    //上报本次快照中未过期的TCP连接数，包含尚未登录的连接，并不是严格的在线用户数
    //登录和断连不在这里即时增减计数，StatusServer看到的负载最多滞后一个扫描周期
    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    auto count_str = std::to_string(session_count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

    //在会话表锁之外清理过期连接，DealExceptionSession内部还会调用ClearSession获取同一把锁
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
