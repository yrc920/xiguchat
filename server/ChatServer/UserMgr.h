#pragma once
// 用户连接管理类, 负责维护用户ID和会话对象的映射关系
//

#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class CSession;
class UserMgr : public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>; //允许Singleton类访问UserMgr的私有成员
public:
	~UserMgr(); //允许Singleton类访问UserMgr的析构函数
	std::shared_ptr<CSession> GetSession(int uid); //根据用户ID获取对应的会话对象
	//将用户ID和会话对象关联起来, 存储在UserMgr中
	void SetUserSession(int uid, std::shared_ptr<CSession> session);
	void RmvUserSession(int uid); //根据用户ID移除对应的会话对象

private:
	UserMgr();
	std::mutex _session_mtx; //保护用户ID和会话对象的映射关系的互斥锁
	//用户ID和会话对象的映射关系, 存储在线用户的会话对象
	std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
