#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "CServer.h"
#include "RequestValidation.h"
#include <charconv>

LogicSystem::LogicSystem() :_b_stop(false), _p_server(nullptr)
{
	RegisterCallBacks(); //注册消息处理函数
	//启动逻辑处理线程，执行DealMsg函数，处理消息队列中的消息
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    Stop();
}

void LogicSystem::Stop()
{
    //停止标志和队列由同一把锁保护，避免消费者检查状态后进入等待却错过停止通知
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _b_stop = true;
    }
	_consume.notify_one(); //发送通知信号，唤醒逻辑处理线程
    if (_worker_thread.joinable()) _worker_thread.join(); //等待剩余消息处理结束，重复Stop时不再join
    _p_server.reset(); //业务线程退出后释放服务器引用，避免服务器存活时间超过它依赖的主io_context
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> unique_lk(_mutex); //使用unique_lock保护消息队列，确保线程安全

	//停服后不再接收新消息；队列达到上限时丢弃当前消息，避免业务处理过慢造成持续积压
	if (_b_stop || _msg_que.size() >= MAX_LOGICQUE) {
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

void LogicSystem::SetServer(std::shared_ptr<CServer> pserver) {
    //主线程在开始处理accept事件前登记服务器，业务线程之后才能检查连接是否仍然有效
    _p_server = pserver;
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::shared_ptr<LogicNode> node;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            //条件变量等待时释放队列锁，收到通知后重新检查条件，也能处理没有实际消息的虚假唤醒
            _consume.wait(lock, [this] { return _b_stop || !_msg_que.empty(); });
            if (_msg_que.empty()) return; //被唤醒且队列为空，说明正在停服且剩余消息已经处理完
            node = _msg_que.front(); //保存当前消息的共享指针，出队后仍能访问会话和消息内容
            _msg_que.pop();
        }
        //离开作用域后释放队列锁，再执行可能阻塞的数据库或RPC调用，避免IO线程也跟着等待
        auto session = node->_session;
        const auto id = node->_recvnode->_msg_id;
        auto handler = _fun_callbacks.find(id);
        //消息排队期间连接可能断开或被踢下线，不能只依赖接收消息时的连接状态
        if (handler == _fun_callbacks.end() || session->IsClosed() ||
            !_p_server || !_p_server->CheckValid(session->GetSessionId())) continue;
        try {
            std::string data(node->_recvnode->_data, node->_recvnode->_cur_len);
            Json::Value root;
            Json::Reader reader;
            //先检查JSON语法，再统一检查字段类型、登录状态和操作者uid，handler只处理通过校验的请求
            auto error = reader.parse(data, root)
                ? ValidateChatRequest(id, root, session->GetUserId()) : Error_Json;
            //会话中有uid并不代表它还是当前登录连接，还要确认UserMgr中的uid确实指向这条会话
            //这样被新连接替代的会话即使还有排队消息，也不能继续执行好友或聊天操作
            if (error == Success && id != MSG_CHAT_LOGIN &&
                UserMgr::GetInstance()->GetSession(session->GetUserId()) != session) {
                error = TokenInvalid;
            }
            if (error != Success) {
                Json::Value response;
                response["error"] = error;
                session->Send(response.toStyledString(), id + 1); //已注册的请求，其响应ID均为请求ID加1
                continue;
            }
            handler->second(session, id, data);
        } catch (const std::exception& error) {
            //异常在单条消息的边界处理，关闭对应连接后继续消费其他消息，避免异常逃出线程导致进程终止
            std::cerr << "Chat request failed: " << error.what() << std::endl;
            session->Close();
            session->DealExceptionSession();
        }
    }
}

void LogicSystem::RegisterCallBacks()
{
	//注册消息处理函数
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	//注册搜索用户信息处理函数
	_fun_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	//注册申请添加好友处理函数
	_fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	_fun_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	_fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[ID_HEART_BEAT_REQ] = std::bind(&LogicSystem::HeartBeatHandler, this,
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
        std::cout << "user uid is " << uid << std::endl;

	Json::Value rtvalue; //创建Json值对象，用于存储返回给客户端的数据
	Defer defer([this, &rtvalue, session]() {
		//将Json值对象转换为字符串形式，准备发送给客户端
		std::string return_str = rtvalue.toStyledString();
		//通过会话对象发送消息给客户端, 消息ID为用户登陆回包
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});

    //会话锁覆盖登录登记过程，断连清理必须等待登记结束后才能读取uid并删除状态
    //锁对象声明在回包Defer之后，因此退出时先解锁再回包，发送失败触发清理时不会重复获取自身持有的锁
    auto lifecycle = session->LockLifecycle();
    if (session->IsClosed()) return; //等待会话锁期间连接可能已经关闭

	//从redis获取用户token是否正确
	std::string uid_str = std::to_string(uid); //将用户ID转换为字符串形式
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

	std::string base_key = USER_BASE_INFO + uid_str; //构建用户基本信息在Redis中的键名
	auto user_info = std::make_shared<UserInfo>(); //初始化用户信息对象
	bool b_base = GetBaseInfo(base_key, uid, user_info); //获取用户基本信息
	//如果获取用户基本信息失败，说明用户ID无效，返回uid无效错误码
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	//将用户基本信息填充到Json值对象中，准备返回给客户端
	rtvalue["uid"] = uid;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;

	//从数据库获取申请列表
	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list); //获取好友申请信息列表
	//如果获取好友申请信息列表成功
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			rtvalue["apply_list"].append(obj); //将好友申请信息添加到Json值对象的apply_list数组中
		}
	}

	//从数据库获取好友列表
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		rtvalue["friend_list"].append(obj); //将好友信息添加到Json值对象的friend_list数组中
	}

	//从配置文件中获取自身服务器的名称
	auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    {
	    //会话锁只保护当前连接，用户锁则用于协调同一uid在不同连接、不同实例上的登录与清理
	    //获取用户锁后，才能检查已有登录位置、通知下线并登记当前会话
	    auto lock_key = LOCK_PREFIX + uid_str;
	    auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
        if (identifier.empty()) { //未获得锁时返回失败，不能继续覆盖共享登录状态
            rtvalue["error"] = ErrorCodes::RPCFailed;
            return;
        }


	    //利用defer解锁
	    Defer defer2([this, identifier, lock_key]() {
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
            });
        //等待分布式锁期间Close仍可标记连接关闭，因此拿到锁后还要再次检查连接是否有效
        if (session->IsClosed() || !_p_server->CheckValid(session->GetSessionId())) {
            rtvalue["error"] = ErrorCodes::TokenInvalid;
            return;
        }
	    //此处判断该用户是否在别处或者本服务器登录

	    std::string uid_ip_value = "";
	    auto uid_ip_key = USERIPPREFIX + uid_str;
	    bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);
	    //位置键表示最近登记的实例，不保证对应连接仍然在线；本实例继续查会话，跨实例发RPC通知
	    if (b_ip) {
	        //获取当前实例名称，与Redis中的位置值比较，判断能否直接从本地找到用户会话
	        auto& cfg = ConfigMgr::Inst();
	        auto self_name = cfg["SelfServer"]["Name"];
	        //如果之前登录的服务器和当前相同，则直接在本服务器踢掉
	        if (uid_ip_value == self_name) {
	            //查找旧有的连接
	            auto old_session = UserMgr::GetInstance()->GetSession(uid);

	            //此处应该发送踢人消息
	            if (old_session) {
	                old_session->NotifyOffline(uid);
	                //移除旧连接的本地登记，使它不能继续处理业务；这里并不直接关闭socket
	                _p_server->ClearSession(old_session->GetSessionId());
	            }
	        }
	        else {
	            //如果不是本服务器，则通知grpc通知其他服务器踢掉
	            //发送通知
                KickUserReq kick_req;
	            kick_req.set_uid(uid);
	            ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
	        }
	    }

	    session->SetUserId(uid); //session绑定用户uid
	    std::string ipkey = USERIPPREFIX + uid_str; //用户所在实例的位置键
	    RedisMgr::GetInstance()->Set(ipkey, server_name); //保存实例名称，供消息路由查找对应的RPC连接池
	    UserMgr::GetInstance()->SetUserSession(uid, session); //将用户ID和会话对象绑定，方便踢人操作
	    std::string uid_session_key = USER_SESSION_PREFIX + uid_str; //为用户设置session的名字
	    //将用户session的名字写入Redis
	    RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());
    }
	return;
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session,
	const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader; //创建Json解析器对象
	Json::Value root; //创建Json值对象，用于存储解析后的数据
	reader.parse(msg_data, root); //解析Json字符串，将结果存储在root对象中
	auto uid_str = root["uid"].asString(); //从解析后的Json对象中获取用户ID字符串
	std::cout << "user SearchInfo uid is  " << uid_str << std::endl;

	Json::Value rtvalue; //创建Json值对象，用于存储返回给客户端的数据
	Defer defer([this, &rtvalue, session]() {
		//将Json值对象转换为字符串形式，准备发送给客户端
		std::string return_str = rtvalue.toStyledString();
		//通过会话对象发送消息给客户端, 消息ID为搜索用户回包
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	bool b_digit = isPureDigit(uid_str); //判断用户ID字符串是否纯数字
	if (b_digit) {
		GetUserByUid(uid_str, rtvalue); //如果用户ID字符串是纯数字，则按照用户ID查询用户信息
	}
	else {
		GetUserByName(uid_str, rtvalue); //如果用户ID字符串不是纯数字，则按照用户名查询用户信息
	}
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session,
	const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader; //创建Json解析器对象
	Json::Value root; //创建Json值对象，用于存储解析后的数据
	reader.parse(msg_data, root); //解析Json字符串，将结果存储在root对象中
	//从解析后的Json对象中获取申请添加好友的相关信息
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
        std::cout << "user uid is " << uid << std::endl;

	Json::Value rtvalue; //创建Json值对象，用于存储返回给客户端的数据
	rtvalue["error"] = ErrorCodes::Success; //默认返回成功状态码
	Defer defer([this, &rtvalue, session]() {
		//将Json值对象转换为字符串形式，准备发送给客户端
		std::string return_str = rtvalue.toStyledString();
		//通过会话对象发送消息给客户端, 消息ID为申请添加好友回包
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});

    if (!MysqlMgr::GetInstance()->AddFriendApply(uid, touid)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }

	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str; //构建目标用户登录ip server的名字在Redis中的键名
	std::string to_ip_value = "";
	//从Redis中获取目标用户的登录ip server的名字
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	//如果Redis中没有目标用户的登录ip server的名字，说明目标用户未登录，直接返回成功状态码，不需要通知对方
	if (!b_ip)
		return;

	//从配置文件中获取自身服务器的名称
	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];

	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>(); //初始化申请人用户信息对象
	bool b_info = GetBaseInfo(base_key, uid, apply_info); //获取申请人用户基本信息

	//如果目标用户在当前服务器上
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid); //从UserMgr中获取目标用户的会话对象
		//如果目标用户的会话对象存在，说明目标用户在线，直接发送通知给对方
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value notify; //创建Json值对象，用于存储通知信息
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			if (b_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
			}
			std::string return_str = notify.toStyledString();
			//通过会话对象发送消息给目标用户, 消息ID为通知添加好友请求
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}
		return;
	}

	AddFriendReq add_req; //创建AddFriendReq消息对象，准备发送通知
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc("");
	//如果获取申请人用户基本信息成功，则将申请人用户的头像、性别和昵称填充到AddFriendReq消息对象中
	if (b_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}

	//通过ChatGrpcClient发送通知添加好友请求给目标用户所在的服务器
	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session,
	const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}


	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});

    if (!b_info) return;
    //DAO在同一事务中检查申请、确认状态并建立双向好友关系，只有提交成功后才通知对方
    if (!MysqlMgr::GetInstance()->AddFriend(uid, touid, back_name)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }

	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}


			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}
		return;
	}

	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);

	//发送通知
	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session,
	const short& msg_id, const std::string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value  arrays = root["text_array"];

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["text_array"] = arrays;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});


	//查询redis 查找touid对应的server ip
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	//直接通知对方有认证通过消息
	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {
			//在内存中则直接发送通知对方
			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}

		return;
	}


	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}

	//发送通知 todo...
	rtvalue["error"] = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue).error();
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session,
    const short& msg_id, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["fromuid"].asInt();
    std::cout << "receive heart beat msg, uid is " << uid << std::endl;
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::isPureDigit(const std::string& str)
{
    if (str.empty()) return false;
	for (unsigned char c : str) {
		//如果字符串中的某个字符不是数字，则返回false，说明字符串不是纯数字
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success; //默认返回成功状态码
	std::string base_key = USER_BASE_INFO + uid_str; //构建用户基本信息在Redis中的键名

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str); //从Redis中获取用户基本信息字符串
	if (b_base) {
		Json::Reader reader; //创建Json解析器对象
		Json::Value root; //创建Json值对象，用于存储解析后的数据
		reader.parse(info_str, root); //解析Json字符串，将结果存储在root对象中
		//从解析后的Json对象中获取用户基本信息
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();

		//将用户基本信息填充到返回值Json对象中，准备返回给客户端
		rtvalue["uid"] = uid;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
	}

	int uid = 0;
    //纯数字字符串也可能超出int范围，from_chars通过错误码报告失败，不会像stoi一样抛出异常
    auto [end, error] = std::from_chars(uid_str.data(), uid_str.data() + uid_str.size(), uid);
    if (error != std::errc{} || end != uid_str.data() + uid_str.size() || uid <= 0) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
	//redis中没有则查询mysql
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(uid); //根据用户ID从MySQL数据库中获取用户信息
	//如果数据库中没有用户信息，则返回uid无效错误码
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	redis_root["icon"] = user_info->icon;
	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString()); //将用户基本信息写入Redis缓存

	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success; //默认返回成功状态码
	std::string base_key = NAME_INFO + name; //构建用户基本信息在Redis中的键名

	//优先查redis中查询用户信息
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str); //从Redis中获取用户基本信息字符串
	if (b_base) {
		Json::Reader reader; //创建Json解析器对象
		Json::Value root; //创建Json值对象，用于存储解析后的数据
		reader.parse(info_str, root); //解析Json字符串，将结果存储在root对象中
		//从解析后的Json对象中获取用户基本信息
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();

		//将用户基本信息填充到返回值Json对象中，准备返回给客户端
		rtvalue["uid"] = uid;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}

	//redis中没有则查询mysql
	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(name); //根据用户名从MySQL数据库中获取用户信息
	//如果数据库中没有用户信息，则返回uid无效错误码
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	//将数据库内容写入redis缓存
	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString()); //将用户基本信息写入Redis缓存

	//返回数据
	rtvalue["uid"] = user_info->uid;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
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
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();

	}
	else
	{
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

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list)
{
	//从mysql获取好友申请列表
	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
	//从mysql获取好友列表
	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}
