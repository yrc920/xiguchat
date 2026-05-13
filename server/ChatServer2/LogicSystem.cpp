#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"

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
	std::cout << "user login uid is  " << uid << " user token  is "
		<< root["token"].asString() << std::endl;
	//从状态服务器获取token匹配是否准确
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());

	Json::Value rtvalue; //创建Json值对象，用于存储返回给客户端的数据
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString(); //将Json值对象转换为字符串形式，准备发送给客户端
		//通过会话对象发送消息给客户端, 消息ID为用户登陆回包
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});

	rtvalue["error"] = rsp.error(); //将状态服务器返回的错误码存储在Json值对象中，准备返回给客户端
	//如果状态服务器返回的错误码不为成功，说明token验证失败
	if (rsp.error() != ErrorCodes::Success)
		return;

	//内存中查询用户信息
	auto find_iter = _users.find(uid);
	//初始化一个用户信息的shared_ptr对象，用于存储查询到的用户信息
	std::shared_ptr<UserInfo> user_info = nullptr;

	//如果内存中没有找到用户信息，说明这是第一次登录或者之前的用户信息已经被清除，需要从数据库中查询用户信息
	if (find_iter == _users.end()) {
		user_info = MysqlMgr::GetInstance()->GetUser(uid); //查询数据库
		//如果数据库中没有找到用户信息，说明用户ID无效，返回错误码给客户端
		if (user_info == nullptr) {
			rtvalue["error"] = ErrorCodes::UidInvalid; //错误码设置为用户ID无效
			return;
		}

		_users[uid] = user_info; //将查询到的用户信息存储在内存中，方便下次登录时快速获取用户信息
	}
	else {
		user_info = find_iter->second; //如果内存中找到了用户信息，直接使用内存中的用户信息进行后续处理
	}

	//将用户ID、token和用户名存储在Json值对象中，准备返回给客户端
	rtvalue["uid"] = uid;
	rtvalue["token"] = rsp.token();
	rtvalue["name"] = user_info->name;
}
