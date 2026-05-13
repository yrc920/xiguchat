#pragma once
// 会话类, 代表一个客户端连接, 负责与客户端通信
//

#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"

namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	tcp::socket& GetSocket(); //返回socket对象的引用，以便服务器可以使用它进行通信
	std::string& GetUuid(); //返回会话的UUID

	void Start(); //启动会话, 开始处理连接
	void Send(char* msg, short max_length, short msgid); //发送消息
	void Send(std::string msg, short msgid); //发送消息
	void Close(); //关闭会话
	std::shared_ptr<CSession> SharedSelf(); //返回一个指向当前对象的shared_ptr
	void AsyncReadBody(int length); //异步读取消息体
	void AsyncReadHead(int total_len); //异步读取消息头

private:
	//读取完整长度
	void asyncReadFull(std::size_t maxLength,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	//读取指定字节数
	void asyncReadLen(std::size_t  read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	//处理写操作完成后的回调函数
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

	tcp::socket _socket; //套接字
	std::string _uuid; //会话唯一标识
	char _data[MAX_LENGTH]; //接收缓冲区
	CServer* _server; //指向服务器对象的指针
	bool _b_close; //会话是否关闭
	std::queue<std::shared_ptr<SendNode> > _send_que; //发送队列
	std::mutex _send_lock; //保护发送队列的互斥锁
	std::shared_ptr<RecvNode> _recv_msg_node; //收到的消息结构
	bool _b_head_parse; //是否已经解析头部
	std::shared_ptr<MsgNode> _recv_head_node; //收到的头部结构
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

private:
	std::shared_ptr<CSession> _session; //会话对象的指针
	std::shared_ptr<RecvNode> _recvnode; //收到的消息结构
};