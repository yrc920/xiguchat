#pragma once
// 会话类, 代表一个客户端连接, 负责与客户端通信
//

#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <atomic>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"

namespace net = boost::asio;            //from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       //from <boost/asio/ip/tcp.hpp>
class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	tcp::socket& GetSocket(); //返回socket对象的引用，以便服务器可以使用它进行通信
	std::string& GetSessionId(); //返回会话ID的引用，以便服务器可以使用它来标识会话
	void SetUserId(int uid); //设置用户ID
	int GetUserId(); //获取用户ID

	void Start(); //启动会话, 开始处理连接
	void Send(char* msg, short max_length, short msgid); //发送消息
	void Send(std::string msg, short msgid); //发送消息
	void Close(); //关闭会话
    bool IsClosed() const { return _b_close.load(); } //查询是否已请求关闭，不代表底层socket已经完成关闭
    //返回持有会话锁的RAII对象，调用方保留该对象期间，登录登记与断连清理不能同时进行
    std::unique_lock<std::mutex> LockLifecycle() { return std::unique_lock<std::mutex>(_lifecycle_mutex); }
	std::shared_ptr<CSession> SharedSelf(); //返回一个指向当前对象的shared_ptr
	void AsyncReadBody(int length); //异步读取消息体
	void AsyncReadHead(int total_len); //异步读取消息头
    void NotifyOffline(int uid); //通知用户离线
    bool IsHeartbeatExpired(std::time_t& now); //判断心跳是否过期
    void UpdateHeartbeat(); //更新心跳
    void DealExceptionSession(); //处理异常连接

private:
    void SendOnIO(std::string msg, short msgid); //只在 socket 所属 IO 线程中执行
	//读取完整长度
	void asyncReadFull(std::size_t maxLength,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	//读取指定字节数
	void asyncReadLen(std::size_t  read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	//处理写操作完成后的回调函数
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

	tcp::socket _socket; //套接字
	std::string _session_id; //连接的 UUID；与登录 token 分开使用
    std::atomic<int> _user_uid; //当前绑定的用户ID，0表示未认证，允许逻辑线程和清理线程读取
	char _data[MAX_LENGTH]; //接收缓冲区
	CServer* _server; //指向服务器对象的指针
	std::atomic<bool> _b_close; //会话是否关闭
	std::queue<std::shared_ptr<SendNode> > _send_que; //仅由所属IO线程访问，队首是正在异步发送的消息
    std::atomic<std::size_t> _pending_sends{0}; //包含尚未进入 IO 线程的发送任务
	std::shared_ptr<RecvNode> _recv_msg_node; //收到的消息结构
	std::shared_ptr<MsgNode> _recv_head_node; //收到的头部结构
    std::atomic<time_t> _last_heartbeat; //最近收到完整消息的时间，供服务器定时器判断连接是否超时
    std::mutex _lifecycle_mutex; //登录登记与断连清理互斥
    std::atomic<bool> _cleanup_started{false}; //断连清理只执行一次
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

private:
	std::shared_ptr<CSession> _session; //会话对象的指针
	std::shared_ptr<RecvNode> _recvnode; //收到的消息结构
};
