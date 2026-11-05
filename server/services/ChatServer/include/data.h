#pragma once
// 数据结构定义, 包含用户信息和好友申请信息的结构体
//

#include <string>

//用户信息结构体
struct UserInfo {
	UserInfo() : name(""), pwd(""), uid(0), email(""), nick(""),
		desc(""), sex(0), icon(""), back("") {}
	std::string name; //用户名
	std::string pwd; //密码
	int uid; //用户ID
	std::string email; //邮箱
	std::string nick; //昵称
	std::string desc; //描述
	int sex; //性别
	std::string icon; //头像
	std::string back; //背景
};

//好友申请信息结构体
struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status)
		:_uid(uid), _name(name), _desc(desc),
		_icon(icon), _nick(nick), _sex(sex), _status(status) {}

	int _uid; //用户ID
	std::string _name; //用户名
	std::string _desc; //描述
	std::string _icon; //头像
	std::string _nick; //昵称
	int _sex; //性别
	int _status; //申请状态，0表示未处理，1表示同意，2表示拒绝
};

//会话列表条目: 本端视角下的一条单聊, peer_* 是会话对面那个人的资料
struct ChatThreadInfo {
	long long thread_id = 0; //会话ID, 由 chat_thread 生成
	int peer_uid = 0; //对方用户ID
	std::string peer_name; //对方用户名
	std::string peer_nick; //对方昵称
	std::string peer_icon; //对方头像
	int peer_sex = 0; //对方性别
};

//一条聊天消息: message_id 来自 chat_message 自增主键, 决定会话内的显示顺序
struct ChatMessageInfo {
	long long message_id = 0; //全局自增的消息ID, 0 表示尚未落库
	long long thread_id = 0; //所属会话ID
	int sender_id = 0; //发送者用户ID
	int recv_id = 0; //接收者用户ID
	std::string client_msg_id; //客户端生成的去重ID, 重发同一条时保持不变
	std::string content; //消息正文
	int status = 0; //消息状态, 当前落库统一为默认值, 未读/已读由客户端展示
};
