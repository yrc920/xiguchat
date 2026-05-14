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
