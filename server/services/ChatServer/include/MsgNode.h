#pragma once
// 消息节点类, 用于存储和处理消息数据
//

#include <boost/asio.hpp>
#include <string>
#include "const.h"

using boost::asio::ip::tcp;
class LogicSystem;

//消息节点类(主要用于解析头部)
class MsgNode
{
public:
	MsgNode(short max_len);
	~MsgNode();
	void Clear();

	short _cur_len; //当前已经接收的长度
	short _total_len; //消息的总长度, 由外部传入, 以便于分配内存
	char* _data; //消息数据
};

//接收消息节点类(储存完整消息)
class RecvNode :public MsgNode {
	friend class LogicSystem;
public:
	RecvNode(short max_len, short msg_id);

private:
	short _msg_id; //消息id, 由外部传入, 以便于逻辑系统根据id分发消息
};

//发送消息节点类
class SendNode :public MsgNode {
	friend class LogicSystem;
public:
	SendNode(const char* msg, short max_len, short msg_id);

private:
	short _msg_id; //消息id, 由外部传入, 以便于逻辑系统根据id分发消息
};