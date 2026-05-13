#pragma once
// 数据结构定义, 用户信息结构体
//

#include <string>

struct UserInfo {
	int uid; //用户ID
	std::string name; //用户名
	std::string email; //邮箱
	std::string pwd; //密码
};