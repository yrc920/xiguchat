#ifndef USERDATA_H
#define USERDATA_H

#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
/******************************************************************************
 *
 * @file       userdata.h
 * @brief      用户数据结构定义，包含搜索结果、好友申请、好友信息等数据结构
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

//用户数据结构
class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon);

    int _uid; //用户ID
    QString _name; //用户名
    QString _nick; //用户昵称
    QString _desc; //用户描述
    int _sex; //用户性别
    QString _icon; //用户头像URL
};

//加好友的申请数据结构
class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex);
    int _from_uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int     _sex;
};

//好友申请信息数据结构
struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc, QString icon, QString nick, int sex, int status) :
        _uid(uid),_name(name),_desc(desc), _icon(icon),_nick(nick),_sex(sex),_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo) : _uid(addinfo->_from_uid),
        _name(addinfo->_name), _desc(addinfo->_desc), _icon(addinfo->_icon),
        _nick(addinfo->_nick),_sex(addinfo->_sex), _status(0) {}

    void SetIcon(QString head){ _icon = head; }

    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};

//用户认证信息数据结构
struct AuthInfo {
    AuthInfo(int uid, QString name, QString nick, QString icon, int sex) :
        _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex){}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
};

//用户认证响应数据结构
struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name, QString peer_nick, QString peer_icon, int peer_sex) :
        _uid(peer_uid),_name(peer_name),_nick(peer_nick), _icon(peer_icon),_sex(peer_sex){}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
};

//好友信息数据结构
struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "") :
        _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex), _last_msg(last_msg){}

    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid), _name(auth->_name) ,_nick(auth->_nick),
        _icon(auth->_icon), _sex(auth->_sex), _last_msg(""){}

    UserInfo(int uid, QString name, QString icon):
        _uid(uid), _name(name), _nick(_name), _icon(icon), _sex(0), _last_msg(""){}

    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid), _name(auth->_name), _nick(auth->_nick),
        _icon(auth->_icon), _sex(auth->_sex), _last_msg(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        _uid(search_info->_uid),_name(search_info->_name),_nick(search_info->_nick),
        _icon(search_info->_icon),_sex(search_info->_sex),_last_msg(""){
    }

    // UserInfo(std::shared_ptr<FriendInfo> friend_info):
    //     _uid(friend_info->_uid),_name(friend_info->_name),_nick(friend_info->_nick),
    //     _icon(friend_info->_icon),_sex(friend_info->_sex),_last_msg(""){
    // }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _last_msg;

};

#endif // USERDATA_H
