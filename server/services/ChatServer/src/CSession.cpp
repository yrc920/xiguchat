#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.h"
#include "RedisMgr.h"
#include <cstdint>
#include <limits>

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _user_uid(0), _server(server), _b_close(false)
{
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()(); //生成一个随机的UUID
	_session_id = boost::uuids::to_string(a_uuid); //将UUID转换为字符串形式
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN); //分配头部节点内存
    _last_heartbeat = std::time(nullptr); //初始化上次接受数据的时间为当前时间
}

CSession::~CSession() {
	std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket; //返回socket对象的引用，以便服务器可以使用它进行通信
}

std::string& CSession::GetSessionId() {
	return _session_id;
}

void CSession::SetUserId(int uid)
{
	_user_uid = uid;
}

int CSession::GetUserId()
{
	return _user_uid;
}

void CSession::Close() {
    //读失败、写失败和心跳超时都可能关闭同一连接，exchange返回旧值，保证只投递一次关闭操作
    if (_b_close.exchange(true)) {
        return;
    }
    //先标记关闭，让其他线程停止投递新消息，再把实际关闭动作交给socket所属的IO线程
    //每个io_context只有一个工作线程，关闭与异步收发的发起操作在同一线程执行，避免并发操作socket
    auto self = shared_from_this();
    boost::asio::post(_socket.get_executor(), [self]() {
        boost::system::error_code ec;
        self->_socket.close(ec); //使用error_code接收错误，避免在异步回调中抛出异常
    });
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	//返回共享当前对象所有权的指针，异步回调持有它期间，会话对象不会被销毁
	return shared_from_this();
}

void CSession::Start() {
    //accept在服务器线程完成，会话的读取由自己的IO线程启动；捕获self保证任务执行前会话不会被销毁
    auto self = shared_from_this();
    boost::asio::post(_socket.get_executor(), [self]() {
        if (!self->IsClosed()) self->AsyncReadHead(HEAD_TOTAL_LEN); //任务排队期间可能已经收到关闭请求
    });
}

void CSession::Send(std::string msg, short msgid)
{
    if (IsClosed()) return; //已关闭的连接不再接受发送任务
    //MsgNode用short保存总长度，消息体加上包头后也不能溢出，必须在长度转换前完成检查
    //否则较大的字符串长度被截断后，分配的内存与实际复制长度可能不一致
    if (msg.empty() || msg.size() > std::numeric_limits<short>::max() - HEAD_TOTAL_LEN) {
        Close();
        DealExceptionSession();
        return;
    }
    //pending_sends同时统计等待进入IO线程和已经进入发送队列的消息，不能只检查_send_que
    //如果IO线程正忙，只限制_send_que仍会让大量发送任务积压在io_context中
    if (_pending_sends.fetch_add(1) >= MAX_SENDQUE) {
        _pending_sends.fetch_sub(1); //本次消息不入队，归还刚占用的计数
        Close();
        DealExceptionSession();
        return;
    }
    auto self = shared_from_this();
    //消息内容移入回调，由回调持有到真正入队，调用方返回后不会留下失效的字符串引用
    boost::asio::post(_socket.get_executor(), [self, msg = std::move(msg), msgid]() mutable {
        self->SendOnIO(std::move(msg), msgid);
    });
}

void CSession::Send(char* msg, short max_length, short msgid)
{
    if (!msg || max_length <= 0) return;
    Send(std::string(msg, static_cast<std::size_t>(max_length)), msgid); //复制指定字节数，复用统一的长度检查和投递流程
}

void CSession::SendOnIO(std::string msg, short msgid)
{
    //发送任务排队后连接仍可能被关闭，这种消息尚未进入_send_que，只归还自身占用的计数
    if (IsClosed()) {
        _pending_sends.fetch_sub(1);
        return;
    }
    const bool writing = !_send_que.empty(); //非空表示已有一条异步写负责推进队列
    _send_que.push(std::make_shared<SendNode>(msg.data(), static_cast<short>(msg.size()), msgid));
    //已有写操作时只入队，后续由HandleWrite续写，避免同时对同一个socket发起多次async_write
    if (writing) return;
    auto& node = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(node->_data, node->_total_len),
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::HandleWrite(const boost::system::error_code& error,
	std::shared_ptr<CSession> shared_self)
{
	try {
		//如果发生错误，输出错误信息
		if (error) {
			std::cout << "handle write failed, error is " << error.what() << std::endl;
			Close(); //关闭连接
            DealExceptionSession(); //处理异常连接
            //async_write完成前必须保留队首缓冲区，因此不能在Close中直接清空发送队列
            //现在写回调已返回结果，可以释放队列中的消息；尚未入队的任务由SendOnIO各自归还计数
            std::queue<std::shared_ptr<SendNode>> empty;
            _pending_sends.fetch_sub(_send_que.size());
            _send_que.swap(empty);
            return;
        }

		_send_que.pop(); //从发送队列中移除已经发送的消息节点
        _pending_sends.fetch_sub(1); //当前消息发送完成，释放一个发送名额

		//如果发送队列中还有消息在等待发送，则继续发送下一个消息
		if (!_send_que.empty() && !IsClosed()) {
			auto& msgnode = _send_que.front(); //获取发送队列中的下一个消息节点
			//异步发送消息数据，当发送完成后会调用HandleWrite函数进行处理
			boost::asio::async_write(_socket,
				boost::asio::buffer(msgnode->_data, msgnode->_total_len),
				std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code : " << e.what() << std::endl;
	}
}

void CSession::AsyncReadBody(int total_len)
{
	auto self = shared_from_this();
	//异步读取消息体数据，当读取完成后会调用回调函数进行处理
	asyncReadFull(total_len, [self, this, total_len](
		const boost::system::error_code& ec, std::size_t bytes_transfered) {
			try {
				//如果发生错误，关闭连接并清除会话
				if (ec) {
					std::cout << "handle read failed, error is " << ec.what() << std::endl;
					Close(); //关闭连接
					DealExceptionSession(); //处理异常连接
					return;
				}

				//如果读取的字节数小于消息长度，说明数据不完整，关闭连接并清除会话
				//封装的asyncReadFull函数会保证读取到指定长度的数据，除非发生错误，这里的判断其实是冗余的
				if (bytes_transfered < static_cast<std::size_t>(total_len)) {
					std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
						<< total_len << "]" << std::endl;
					Close(); //关闭连接
                DealExceptionSession(); //关闭并清理异常连接
					return;
				}

			    //判断连接无效
                if (!_server->CheckValid(_session_id)) {
                    Close();
                    return;
                }

				memcpy(_recv_msg_node->_data, _data, bytes_transfered); //将读取到的数据复制到消息节点内
				_recv_msg_node->_cur_len += bytes_transfered; //更新消息节点的当前长度
				//在消息数据末尾添加字符串结束符，确保消息数据可以被正确解析为字符串
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				//不记录原始消息体，登录请求包含 token。

                UpdateHeartbeat(); //任意完整请求都会刷新活跃时间，不限于专门的心跳消息
				//此处将消息投递到逻辑队列中
				LogicSystem::GetInstance()->PostMsgToQue(
					std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
				
				AsyncReadHead(HEAD_TOTAL_LEN); //继续监听头部接受事件
			}
			catch (std::exception& e) {
				std::cout << "Exception code is " << e.what() << std::endl;
			}
		});
}

void CSession::AsyncReadHead(int /*total_len*/)
{
	auto self = shared_from_this();
	//异步读取头部数据，当读取完成后会调用回调函数进行处理
	asyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec,
		std::size_t bytes_transfered) {
		try {
			//如果发生错误，关闭连接并清除会话
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << std::endl;
				Close(); //关闭连接
				DealExceptionSession(); //处理异常连接
				return;
			}

			//如果读取的字节数小于头部长度，说明数据不完整，关闭连接并清除会话
			//封装的asyncReadFull函数会保证读取到指定长度的数据，除非发生错误，这里的判断其实是冗余的
			if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< HEAD_TOTAL_LEN << "]" << std::endl;
				Close(); //关闭连接
                DealExceptionSession(); //关闭并清理异常连接
				return;
			}

		    //判断连接无效
            if (!_server->CheckValid(_session_id)) {
                Close();
                return;
            }

			_recv_head_node->Clear(); //清空头部节点数据
			memcpy(_recv_head_node->_data, _data, bytes_transfered); //将读取到的数据复制到头部节点内

			//获取头部MSGID数据
			std::uint16_t msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN); //从头部数据中提取消息ID
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
            //这里先限制消息ID的数值范围，具体ID是否注册了处理函数由LogicSystem判断
			if (msg_id == 0 || msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				Close();
                DealExceptionSession(); //关闭并清理异常连接
				return;
			}

			//获取消息长度数据
			std::uint16_t msg_len = 0;
			//从头部数据中提取消息长度
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << std::endl;
            //用无符号数读取包长，避免高位为1时被当成负数而绕过上限检查
            //拒绝空包和超过接收缓冲区的消息体，检查通过后才分配RecvNode并启动读取
			if (msg_len == 0 || msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << std::endl;
				Close();
                DealExceptionSession(); //关闭并清理异常连接
				return;
			}

			//分配消息节点内存，并设置消息长度和消息ID
			_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id); 
			AsyncReadBody(msg_len); //继续异步读取消息体，准备接收数据
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});
}

void CSession::NotifyOffline(int uid)
{
    Json::Value rtvalue; //创建一个Json对象，用于存储返回的结果
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    std::string return_str = rtvalue.toStyledString(); //将Json对象转换为字符串形式
    Send(return_str, ID_NOTIFY_OFF_LINE_REQ); //调用Send函数发送离线通知消息给客户端
    return;
}

//读取完整长度
void CSession::asyncReadFull(std::size_t maxLength,
	std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	::memset(_data, 0, MAX_LENGTH); //清空接收缓冲区
	asyncReadLen(0, maxLength, handler); //调用asyncReadLen函数保证读取到指定长度的数据
}

//读取指定字节数
void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
	auto self = shared_from_this();
	//异步读取数据到接收缓冲区中
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
		[read_len, total_len, handler, self](
			const boost::system::error_code& ec, std::size_t  bytesTransfered) {
			//如果发生错误
			if (ec) {
				std::cout << "handle read failed, error is " << ec.what() << std::endl;
				handler(ec, read_len + bytesTransfered); //调用回调函数，传入错误码和实际读取的长度
				return;
			}

			//如果读取的字节数加上已经读取的长度大于等于总长度，说明已经读取到完整的数据
			if (read_len + bytesTransfered >= total_len) {
				handler(ec, read_len + bytesTransfered); //调用回调函数，传入错误码和实际读取的长度
				return;
			}

			//没有错误，且长度不足则继续读取
			self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
		});
}

bool CSession::IsHeartbeatExpired(std::time_t& now) {
    double diff_sec = std::difftime(now, _last_heartbeat);
    if (diff_sec > HEART_THRESHOLD) {
        std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
        return true;
    }
    return false;
}

void CSession::UpdateHeartbeat()
{
    time_t now = std::time(nullptr);
    _last_heartbeat = now;
}

void CSession::DealExceptionSession()
{
    //登录登记与断连清理共用会话锁，保证读取uid时登录流程已结束，避免刚清理完又登记为在线
    auto lifecycle = LockLifecycle();
    if (_cleanup_started.exchange(true)) return; //多条异常路径只允许执行一次清理
    auto self = shared_from_this();
    const int uid = GetUserId();
    //未认证连接没有Redis登录状态，直接清理本地记录，不为uid为0的连接争抢同一把用户锁
    if (uid == 0) {
        _server->ClearSession(_session_id);
        return;
    }
    auto uid_str = std::to_string(uid);
    auto lock_key = LOCK_PREFIX + uid_str;
    auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

    //无论获取锁或查询Redis是否成功，都要移除本地会话；释放锁时使用identifier校验持有者
    Defer defer([identifier, lock_key, self, this]() {
        _server->ClearSession(_session_id);
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });

    //没有拿到用户锁就不能删除共享登录状态，只执行上面的本地清理
    if (identifier.empty())
        return;

    std::string redis_session_id = "";
    auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
    if (!bsuccess)
        return;

    if (redis_session_id != _session_id) {
        //Redis已经指向另一条连接，可能是同实例或跨实例重新登录，不能删除新连接的状态
        return;
    }

    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    //清除用户登录信息
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
}


LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	: _session(session), _recvnode(recvnode) {

}
