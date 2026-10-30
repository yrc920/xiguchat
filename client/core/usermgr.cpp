#include "usermgr.h"

#include <QDebug>

UserMgr::UserMgr() : _user_info(nullptr)
{

}

UserMgr::~UserMgr() {}

void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfo()
{
    return _user_info;
}

int UserMgr::GetUid()
{
    return _user_info ? _user_info->_uid : 0;
}

void UserMgr::AppendApplyList(QJsonArray array)
{
    emit ApplyListAboutToChange();
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue &value : array) {
        //从JSON对象中提取好友申请信息
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto status = value["status"].toInt();
        //将好友申请信息封装成ApplyInfo对象的智能指针
        auto info = std::make_shared<ApplyInfo>(uid, name, desc, icon, nick, sex, status);
        _apply_list.push_back(info); //将好友申请信息添加到申请列表中
    }

    emit ApplyListChanged();
}

void UserMgr::AppendFriendList(QJsonArray array) {
    emit FriendListAboutToChange();
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue& value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto back = value["back"].toString();

        auto info = std::make_shared<FriendInfo>(uid, name, nick, icon, sex, desc, back);
        _friend_list.push_back(info);
        _friend_map.insert(uid, info);
    }

    emit FriendListChanged();
}

bool UserMgr::AlreadyApply(int uid)
{
    for(auto& apply: _apply_list){
        //如果申请列表中已经存在来自指定用户ID的好友申请, 则返回true, 表示已经有好友申请
        if(apply->_uid == uid){
            return true;
        }
    }
    return false;
}

void UserMgr::AddApplyList(std::shared_ptr<ApplyInfo> app)
{
    emit ApplyListAboutToChange();
    _apply_list.push_back(app);
    emit ApplyListChanged();
}

void UserMgr::SetApplyStatus(int uid, int status)
{
    for (auto &apply : _apply_list) {
        if (apply->_uid != uid || apply->_status == status)
            continue;
        emit ApplyListAboutToChange();
        apply->_status = status;
        emit ApplyListChanged();
        return;
    }
}

bool UserMgr::CheckFriendById(int uid)
{
    auto iter = _friend_map.find(uid);
    if(iter == _friend_map.end()){
        return false;
    }
    return true;
}

void UserMgr::AddFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_rsp);
    if (_friend_map.contains(friend_info->_uid))
        return;
    emit FriendListAboutToChange();
    _friend_map[friend_info->_uid] = friend_info;
    _friend_list.push_back(friend_info); //列表与索引必须同时维护，否则新好友不会出现在列表里
    emit FriendListChanged();
}

void UserMgr::AddFriend(std::shared_ptr<AuthInfo> auth_info)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_info);
    if (_friend_map.contains(friend_info->_uid))
        return;
    emit FriendListAboutToChange();
    _friend_map[friend_info->_uid] = friend_info;
    _friend_list.push_back(friend_info);
    emit FriendListChanged();
}

void UserMgr::AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs)
{
    auto find_iter = _friend_map.find(friend_id);
    if(find_iter == _friend_map.end()){
        qDebug()<<"append friend uid  " << friend_id << " not found";
        return;
    }

    find_iter.value()->AppendChatMsgs(msgs);
    emit FriendChatChanged(friend_id);
}

int UserMgr::FriendCount() const
{
    return static_cast<int>(_friend_list.size());
}

std::shared_ptr<FriendInfo> UserMgr::FriendAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(_friend_list.size()))
        return nullptr;
    return _friend_list[index];
}

std::shared_ptr<FriendInfo> UserMgr::GetFriendById(int uid)
{
    auto find_it = _friend_map.find(uid);
    if(find_it == _friend_map.end()){
        return nullptr;
    }

    return *find_it;
}

int UserMgr::ApplyCount() const
{
    return static_cast<int>(_apply_list.size());
}

std::shared_ptr<ApplyInfo> UserMgr::ApplyAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(_apply_list.size()))
        return nullptr;
    return _apply_list[index];
}

void UserMgr::ClearSession()
{
    emit ApplyListAboutToChange();
    emit FriendListAboutToChange();
    _user_info.reset();
    _apply_list.clear();
    _friend_list.clear();
    _friend_map.clear();

    emit ApplyListChanged();
    emit FriendListChanged();
}
