#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _server(server), _b_close(false), _b_head_parse(false)
{
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()(); //生成一个随机的UUID
	_uuid = boost::uuids::to_string(a_uuid); //将UUID转换为字符串形式
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN); //分配头部节点内存
}

CSession::~CSession() {
	std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket() {
	return _socket; //返回socket对象的引用，以便服务器可以使用它进行通信
}

std::string& CSession::GetUuid() {
	return _uuid; //返回会话的UUID
}

void CSession::Close() {
	_socket.close(); //关闭套接字
	_b_close = true; //标记会话为关闭状态
}

std::shared_ptr<CSession>CSession::SharedSelf() {
	return shared_from_this(); //返回一个指向当前对象的shared_ptr，允许在异步操作中安全地引用当前会话对象
}

void CSession::Start() {
	AsyncReadHead(HEAD_TOTAL_LEN); //开始异步读取消息头部，准备接收数据
}

void CSession::Send(std::string msg, short msgid)
{
	std::lock_guard<std::mutex> lock(_send_lock); //使用互斥锁保护发送队列，确保线程安全
	int send_que_size = _send_que.size(); //获取当前发送队列的大小
	//如果发送队列的大小超过了预设的最大值，输出警告信息并返回，避免过多的消息积压在队列中
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	//将消息封装成SendNode对象，并添加到发送队列中
	_send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));

	//如果发送队列中已经有消息在等待发送，则不需要立即发送新消息，直接返回即可
	if (send_que_size > 0)
		return;

	//如果发送队列中没有消息在等待发送，则立即发送新消息
	auto& msgnode = _send_que.front(); //获取发送队列中的第一个消息节点
	//异步发送消息数据，当发送完成后会调用HandleWrite函数进行处理
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock); //使用互斥锁保护发送队列，确保线程安全
	int send_que_size = _send_que.size(); //获取当前发送队列的大小
	//如果发送队列的大小超过了预设的最大值，输出警告信息并返回，避免过多的消息积压在队列中
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	//将消息封装成SendNode对象，并添加到发送队列中
	_send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
	//如果发送队列中已经有消息在等待发送，则不需要立即发送新消息，直接返回即可
	if (send_que_size > 0)
		return;
	
	//如果发送队列中没有消息在等待发送，则立即发送新消息
	auto& msgnode = _send_que.front(); //获取发送队列中的第一个消息节点
	//异步发送消息数据，当发送完成后会调用HandleWrite函数进行处理
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
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
			_server->ClearSession(_uuid); //清除会话
		}
		
		std::lock_guard<std::mutex> lock(_send_lock); //使用互斥锁保护发送队列，确保线程安全
		_send_que.pop(); //从发送队列中移除已经发送的消息节点

		//如果发送队列中还有消息在等待发送，则继续发送下一个消息
		if (!_send_que.empty()) {
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
					_server->ClearSession(_uuid); //清除会话
					return;
				}

				//如果读取的字节数小于消息长度，说明数据不完整，关闭连接并清除会话
				//封装的asyncReadFull函数会保证读取到指定长度的数据，除非发生错误，这里的判断其实是冗余的
				if (bytes_transfered < total_len) {
					std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
						<< total_len << "]" << std::endl;
					Close(); //关闭连接
					_server->ClearSession(_uuid); //清除会话
					return;
				}

				memcpy(_recv_msg_node->_data, _data, bytes_transfered); //将读取到的数据复制到消息节点内
				_recv_msg_node->_cur_len += bytes_transfered; //更新消息节点的当前长度
				//在消息数据末尾添加字符串结束符，确保消息数据可以被正确解析为字符串
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				std::cout << "receive data is " << _recv_msg_node->_data << std::endl;

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

void CSession::AsyncReadHead(int total_len)
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
				_server->ClearSession(_uuid); //清除会话
				return;
			}

			//如果读取的字节数小于头部长度，说明数据不完整，关闭连接并清除会话
			//封装的asyncReadFull函数会保证读取到指定长度的数据，除非发生错误，这里的判断其实是冗余的
			if (bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
					<< HEAD_TOTAL_LEN << "]" << std::endl;
				Close(); //关闭连接
				_server->ClearSession(_uuid); //清除会话
				return;
			}

			_recv_head_node->Clear(); //清空头部节点数据
			memcpy(_recv_head_node->_data, _data, bytes_transfered); //将读取到的数据复制到头部节点内

			//获取头部MSGID数据
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN); //从头部数据中提取消息ID
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
			//如果MSGID超过最大长度，说明消息格式不合法，关闭连接并清除会话
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				_server->ClearSession(_uuid); //清除会话
				return;
			}

			//获取消息长度数据
			short msg_len = 0;
			//从头部数据中提取消息长度
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << std::endl;
			//如果消息长度超过最大长度，说明消息格式不合法，关闭连接并清除会话
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << std::endl;
				_server->ClearSession(_uuid); //清除会话
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

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	: _session(session), _recvnode(recvnode) {

}
