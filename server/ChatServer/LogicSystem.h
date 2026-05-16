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
	//登录处理函数, 处理用户登录请求，验证用户身份并返回登录结果
	void LoginHandler(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
	//搜索用户信息处理函数，处理用户搜索请求并返回搜索结果
	void SearchInfo(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
	void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);

	bool isPureDigit(const std::string& str); //判断字符串是否纯数字
	//根据用户ID查询用户信息，并将结果填充到Json值对象中
	void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
	//根据用户名查询用户信息，并将结果填充到Json值对象中
	void GetUserByName(std::string name, Json::Value& rtvalue);
	//获取用户基本信息
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

	std::thread _worker_thread; //逻辑处理线程
	std::queue<std::shared_ptr<LogicNode>> _msg_que; //逻辑消息队列
	std::mutex _mutex; //保护消息队列的互斥锁
	std::condition_variable _consume; //条件变量，用于通知逻辑处理线程有新消息到来
	bool _b_stop; //标志逻辑系统是否停止，控制逻辑处理线程的退出

	//消息ID到处理函数的映射表，用于根据消息ID分发消息到对应的处理函数
	std::map<short, FunCallBack> _fun_callbacks;
};
