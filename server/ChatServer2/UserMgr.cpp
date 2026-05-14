#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"

UserMgr::UserMgr() {}

UserMgr:: ~UserMgr() {
	_uid_to_session.clear(); //清空用户ID和会话对象的映射关系, 释放所有会话对象的资源
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx); //加锁保护用户ID和会话对象的映射关系
	auto iter = _uid_to_session.find(uid); //在映射关系中查找指定用户ID对应的会话对象
	//如果没有找到对应的会话对象, 返回nullptr
	if (iter == _uid_to_session.end())
		return nullptr;

	return iter->second; //返回找到的会话对象的shared_ptr
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(_session_mtx); //加锁保护用户ID和会话对象的映射关系
	_uid_to_session[uid] = session; //将指定用户ID和会话对象的shared_ptr存储在映射关系中
}

void UserMgr::RmvUserSession(int uid)
{
	auto uid_str = std::to_string(uid);
	//因为再次登录可能是其他服务器，所以会造成本服务器删除key，其他服务器注册key的情况
	//可能其他服务登录，本服删除key造成找不到key的情况

	//RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	{
		std::lock_guard<std::mutex> lock(_session_mtx); //加锁保护用户ID和会话对象的映射关系
		_uid_to_session.erase(uid); //从映射关系中移除指定用户ID对应的会话对象, 释放该会话对象的资源
	}
}
