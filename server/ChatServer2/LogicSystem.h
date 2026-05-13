#pragma once
// 逻辑系统类, 负责处理业务逻辑
//

#include "Singleton.h"
#include "CSession.h"
#include "data.h"
#include "const.h"
#include <queue>
#include <thread>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <map>
#include <unordered_map>

//定义一个函数类型，用于处理不同消息ID的回调函数，参数包括会话对象、消息ID和消息数据
typedef std::function<void(std::shared_ptr<CSession>,
	const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>; //允许Singleton类访问LogicSystem的私有成员
public:
	~LogicSystem(); //使基类析构时可以访问派生类的析构函数
	void PostMsgToQue(std::shared_ptr<LogicNode> msg); //将消息节点添加到消息队列中，供逻辑处理线程处理

private:
	LogicSystem();
	void DealMsg(); //逻辑处理线程执行的函数，负责从消息队列中取出消息并调用对应的处理函数进行处理
	void RegisterCallBacks(); //注册消息处理函数
	//处理用户登录消息的函数，解析消息数据并进行相应的处理
	void LoginHandler(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);

	std::thread _worker_thread; //逻辑处理线程
	std::queue<std::shared_ptr<LogicNode>> _msg_que; //逻辑消息队列
	std::mutex _mutex; //保护消息队列的互斥锁
	std::condition_variable _consume; //条件变量，用于通知逻辑处理线程有新消息到来
	bool _b_stop; //标志逻辑系统是否停止，控制逻辑处理线程的退出

	//消息ID到处理函数的映射表，用于根据消息ID分发消息到对应的处理函数
	std::map<short, FunCallBack> _fun_callbacks;
	//用户信息表，使用unordered_map以uid为键，存储用户信息的shared_ptr，方便快速查找和管理用户数据
	std::unordered_map<int, std::shared_ptr<UserInfo>> _users;
};
