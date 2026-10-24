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

void UserMgr::RmvUserSession(int uid, std::string session_id)
{
    std::lock_guard<std::mutex> lock(_session_mtx); //加锁保护用户ID和会话对象的映射关系
    auto iter = _uid_to_session.find(uid); //在映射关系中查找指定用户ID对应的会话对象
    //没有找到对应的会话对象, 直接返回
    if (iter == _uid_to_session.end())
        return;

    auto session_id_ = iter->second->GetSessionId(); //获取当前会话对象的会话ID
    //不相等说明是其他地方登录了
    if (session_id_ != session_id)
        return;

    _uid_to_session.erase(uid); //从映射关系中移除指定用户ID对应的会话对象, 释放会话对象的资源
}
