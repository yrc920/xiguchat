#ifndef USERDATA_H
#define USERDATA_H

#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
#include <utility>
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

struct TextChatData;

//用户认证信息数据结构
struct AuthInfo {
    AuthInfo(int uid, QString name, QString nick, QString icon, int sex, qint64 thread_id = 0,
             qint64 message_id = 0, QString welcome_msg_id = {}) :
        _uid(uid), _name(name), _nick(nick), _icon(icon), _sex(sex), _thread_id(thread_id),
        _message_id(message_id), _welcome_msg_id(std::move(welcome_msg_id)){}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    qint64 _thread_id = 0;
    qint64 _message_id = 0;
    QString _welcome_msg_id;
    std::vector<std::shared_ptr<TextChatData>> _welcome_msgs;
};

//用户认证响应数据结构
struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name, QString peer_nick, QString peer_icon,
            int peer_sex, qint64 thread_id = 0, qint64 message_id = 0,
            QString welcome_msg_id = {}) :
        _uid(peer_uid),_name(peer_name),_nick(peer_nick), _icon(peer_icon),_sex(peer_sex),
        _thread_id(thread_id),_message_id(message_id),
        _welcome_msg_id(std::move(welcome_msg_id)){}

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    qint64 _thread_id = 0;
    qint64 _message_id = 0;
    QString _welcome_msg_id;
    std::vector<std::shared_ptr<TextChatData>> _welcome_msgs;
};

enum class ChatMessageStatus : int {
    Sending = 0,
    Sent = 1,
    Failed = 2,
    Unread = 3,
    Read = 4,
};

struct ChatThreadData {
    explicit ChatThreadData(qint64 thread_id = 0) : _thread_id(thread_id) {}

    qint64 _thread_id = 0;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct FriendInfo {
    FriendInfo(int uid, QString name, QString nick, QString icon,
               int sex, QString desc, QString back, QString last_msg=""):_uid(uid),
        _name(name),_nick(nick),_icon(icon),_sex(sex),_desc(desc),
        _back(back),_last_msg(last_msg),_chat_thread(std::make_shared<ChatThreadData>()){}

    FriendInfo(std::shared_ptr<AuthInfo> auth_info):_uid(auth_info->_uid),
        _nick(auth_info->_nick),_icon(auth_info->_icon),_name(auth_info->_name),
        _sex(auth_info->_sex),_chat_thread(std::make_shared<ChatThreadData>(auth_info->_thread_id))
    {
        _chat_thread->_chat_msgs = auth_info->_welcome_msgs;
    }

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):_uid(auth_rsp->_uid),
        _nick(auth_rsp->_nick),_icon(auth_rsp->_icon),_name(auth_rsp->_name),
        _sex(auth_rsp->_sex),_chat_thread(std::make_shared<ChatThreadData>(auth_rsp->_thread_id))
    {
        _chat_thread->_chat_msgs = auth_rsp->_welcome_msgs;
    }

    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec);

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
    QString _back;
    QString _last_msg;
    std::shared_ptr<ChatThreadData> _chat_thread;
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

    UserInfo(std::shared_ptr<FriendInfo> friend_info):
        _uid(friend_info->_uid),_name(friend_info->_name),_nick(friend_info->_nick),
        _icon(friend_info->_icon),_sex(friend_info->_sex),_last_msg("")
    {
        _chat_msgs = friend_info->_chat_thread->_chat_msgs;
    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _last_msg;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

struct TextChatData{
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid,
                 qint64 message_id = 0, qint64 thread_id = 0,
                 ChatMessageStatus status = ChatMessageStatus::Sending)
        :_msg_id(msg_id),_msg_content(msg_content),_from_uid(fromuid),_to_uid(touid),
         _message_id(message_id),_thread_id(thread_id),_status(status){
    }
    QString _msg_id;
    qint64 _message_id = 0;
    qint64 _thread_id = 0;
    ChatMessageStatus _status = ChatMessageStatus::Sending;
    QString _msg_content;
    QString _imageSource; // 仅用于本地预览，不进入文本协议
    int _imageWidth = 0;
    int _imageHeight = 0;
    int _from_uid;
    int _to_uid;
};

struct TextChatMsg{
    TextChatMsg(int fromuid, int touid, QJsonArray arrays, qint64 thread_id = 0):
        _from_uid(fromuid),_to_uid(touid),_thread_id(thread_id){
        for(auto  msg_data : arrays){
            auto msg_obj = msg_data.toObject();
            auto content = msg_obj["content"].toString();
            auto msgid = msg_obj["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content,fromuid, touid,
                msg_obj["message_id"].toVariant().toLongLong(),
                msg_obj["thread_id"].toVariant().toLongLong() > 0
                    ? msg_obj["thread_id"].toVariant().toLongLong() : thread_id,
                static_cast<ChatMessageStatus>(msg_obj["status"].toInt(
                    static_cast<int>(ChatMessageStatus::Sent))));
            _chat_msgs.push_back(msg_ptr);
        }
    }
    int _to_uid;
    int _from_uid;
    qint64 _thread_id = 0;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

#endif // USERDATA_H
