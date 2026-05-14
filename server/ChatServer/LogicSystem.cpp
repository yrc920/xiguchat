#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"

LogicSystem::LogicSystem() :_b_stop(false)
{
	RegisterCallBacks(); //注册消息处理函数
	//启动逻辑处理线程，执行DealMsg函数，处理消息队列中的消息
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
	_b_stop = true; //设置停止标志，通知逻辑处理线程退出
	_consume.notify_one(); //发送通知信号，唤醒逻辑处理线程
	_worker_thread.join(); //等待逻辑处理线程退出，确保资源的正确释放
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> unique_lk(_mutex); //使用unique_lock保护消息队列，确保线程安全

	//如果消息队列的大小超过了预设的最大值，输出警告信息并返回，避免过多的消息积压在队列中
	if (_msg_que.size() > MAX_LOGICQUE) {
		std::cout << "logic system msg que fulled, size is " << MAX_LOGICQUE << std::endl;
		return;
	}

	_msg_que.push(msg); //将消息添加到消息队列中

	//如果队列由空变为非空，则发送通知信号
	if (_msg_que.size() == 1) {
		unique_lk.unlock(); //释放锁，允许逻辑处理线程访问消息队列
		_consume.notify_one(); //发送通知信号，唤醒逻辑处理线程
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex); //使用unique_lock保护消息队列，确保线程安全
		//判断队列为空则用条件变量阻塞等待，并释放锁
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//如果是关闭状态，把所有逻辑执行完后则退出循环
		if (_b_stop) {
			//继续处理完队列中的消息
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front(); //获取队列中的第一个消息节点
				std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
				//根据消息ID查找对应的处理函数
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				//如果没有找到对应的处理函数
				if (call_back_iter == _fun_callbacks.end()) {
					_msg_que.pop(); //从队列中移除当前消息节点
					continue;
				}

				//调用对应的处理函数，传入会话对象、消息ID和消息数据
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop(); //从队列中移除当前消息节点
			}
			break;
		}

		//如果没有停服，说明队列中有数据
		auto msg_node = _msg_que.front(); //获取队列中的第一个消息节点
		std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
		//根据消息ID查找对应的处理函数
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		//如果没有找到对应的处理函数，说明这个消息ID没有注册处理函数
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop(); //从队列中移除当前消息节点
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}

		//调用对应的处理函数，传入会话对象、消息ID和消息数据
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop(); //从队列中移除当前消息节点
	}
}

void LogicSystem::RegisterCallBacks()
{
	//注册消息处理函数
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session,
	const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader; //创建Json解析器对象
	Json::Value root; //创建Json值对象，用于存储解析后的数据
	reader.parse(msg_data, root); //解析Json字符串，将结果存储在root对象中

	auto uid = root["uid"].asInt(); //从解析后的Json对象中获取用户ID
	auto token = root["token"].asString(); //从解析后的Json对象中获取用户token
	std::cout << "user login uid is  " << uid << " user token  is "
		<< token << std::endl;

	Json::Value rtvalue; //创建Json值对象，用于存储返回给客户端的数据
	Defer defer([this, &rtvalue, session]() {
		//将Json值对象转换为字符串形式，准备发送给客户端
		std::string return_str = rtvalue.toStyledString();
		//通过会话对象发送消息给客户端, 消息ID为用户登陆回包
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});

	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value); //从Redis中获取用户token
	//如果Redis中没有用户token，说明用户ID无效，返回uid无效错误码
	if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	//如果Redis中的用户token与客户端提供的token不匹配，说明token无效，返回token无效错误码
	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		return;
	}

	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>(); //初始化用户信息对象
	bool b_base = GetBaseInfo(base_key, uid, user_info); //获取用户基本信息
	//如果获取用户基本信息失败，说明用户ID无效，返回uid无效错误码
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	//将用户基本信息填充到Json值对象中，准备返回给客户端
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

	//从配置文件中获取自身服务器的名称
	auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
	//从Redis中获取当前服务器的登录数量
	auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;

	//如果Redis中有登录数量记录
	if (!rd_res.empty()) {
		count = std::stoi(rd_res); //将登录数量字符串转换为整数
	}

	count++; //登录数量加1
	auto count_str = std::to_string(count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str); //将更新后的登录数量写回Redis
	session->SetUserId(uid); //session绑定用户uid
	std::string ipkey = USERIPPREFIX + uid_str; //为用户设置登录ip server的名字
	RedisMgr::GetInstance()->Set(ipkey, server_name); //将用户登录ip server的名字写入Redis
	//uid和session绑定管理, 方便以后踢人操作
	UserMgr::GetInstance()->SetUserSession(uid, session);

	return;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str); //从Redis中获取用户信息
	//如果Redis中有用户信息，则解析Json字符串并填充用户信息对象
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is "
			<< userinfo->email << std::endl;
	}
	else {
		//redis中没有则查询mysql
		//查询数据库
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		//如果数据库中没有用户信息，则返回false，表示用户ID无效
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info; //将数据库查询到的用户信息赋值给传入的用户信息对象

		//将数据库内容写入redis缓存
		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}
