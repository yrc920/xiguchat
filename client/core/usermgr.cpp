#include "usermgr.h"

#include "global.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>

namespace {
constexpr qint64 MaxAvatarBytes = 4 * 1024 * 1024;

bool isAvatarPng(const QByteArray &png)
{
    static const QByteArray signature("\x89PNG\r\n\x1a\n", 8);
    return png.size() > signature.size() && png.size() <= MaxAvatarBytes
        && png.startsWith(signature);
}

QString avatarSource(const QByteArray &png)
{
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}
}

UserMgr::UserMgr() : _user_info(nullptr)
{

}

UserMgr::~UserMgr() {}

void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    const QString previousIcon = GetIcon();
    _user_info = user_info;
    _localIcon.clear();
    const QString path = LocalAvatarPath();
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.size() <= MaxAvatarBytes) {
            const QByteArray png = file.read(MaxAvatarBytes + 1);
            if (isAvatarPng(png))
                _localIcon = avatarSource(png);
        }
    }
    if (GetIcon() != previousIcon)
        emit UserIconChanged();
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfo()
{
    return _user_info;
}

int UserMgr::GetUid()
{
    return _user_info ? _user_info->_uid : 0;
}

QString UserMgr::LocalAvatarPath()
{
    const int uid = GetInstance()->GetUid();
    if (uid <= 0)
        return {};

    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty())
        return {};

    const QUrl gateway(gate_url_prefix);
    const QString endpoint = gateway.host().toLower() + QLatin1Char(':')
        + QString::number(gateway.port(gateway.scheme() == QStringLiteral("https") ? 443 : 80));
    const QString scope = QString::fromLatin1(
        QCryptographicHash::hash(endpoint.toUtf8(), QCryptographicHash::Sha256).toHex());
    return QDir(root).filePath(QStringLiteral("avatars/%1/%2/head.png").arg(scope).arg(uid));
}

QString UserMgr::GetIcon() const
{
    if (!_user_info)
        return {};
    return _localIcon.isEmpty() ? _user_info->_icon : _localIcon;
}

bool UserMgr::SetLocalAvatar(const QByteArray &png)
{
    if (GetUid() <= 0 || !isAvatarPng(png))
        return false;

    const QString source = avatarSource(png);
    if (_localIcon != source) {
        _localIcon = source;
        emit UserIconChanged();
    }
    return true;
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

void UserMgr::SetChatThreads(const QJsonArray &threads)
{
    emit FriendListAboutToChange();
    for (auto &friend_info : _friend_list)
        friend_info->_chat_thread->_thread_id = 0;
    AppendChatThreads(threads);
    emit FriendListChanged();
}

void UserMgr::AppendChatThreads(const QJsonArray &threads)
{
    for (const auto &value : threads) {
        const QJsonObject item = value.toObject();
        const int uid = item.value("uid").toInt();
        auto info = GetFriendById(uid);
        if (!info)
            continue;
        info->_chat_thread->_thread_id = item.value("thread_id").toVariant().toLongLong();
    }
}

void UserMgr::RefreshFriendList()
{
    emit FriendListAboutToChange();
    emit FriendListChanged();
}

void UserMgr::SetThreadId(int friend_id, qint64 thread_id)
{
    auto info = GetFriendById(friend_id);
    _pending_private_chats.remove(friend_id);
    if (thread_id <= 0 || !info || info->_chat_thread->_thread_id == thread_id)
        return;
    emit FriendListAboutToChange();
    info->_chat_thread->_thread_id = thread_id;
    emit FriendListChanged();
}

bool UserMgr::HasPendingPrivateChat(int friend_id) const
{
    return _pending_private_chats.value(friend_id, false);
}

void UserMgr::MarkPrivateChatPending(int friend_id, bool pending)
{
    if (pending)
        _pending_private_chats[friend_id] = true;
    else
        _pending_private_chats.remove(friend_id);
}

void UserMgr::UpdateChatMessage(int friend_id, const QString &msg_id, qint64 message_id,
                                ChatMessageStatus status)
{
    auto info = GetFriendById(friend_id);
    if (!info)
        return;
    for (auto &message : info->_chat_thread->_chat_msgs) {
        if (message->_msg_id != msg_id)
            continue;
        if (message_id > 0)
            message->_message_id = message_id;
        message->_status = status;
        info->AppendChatMsgs({message});
        emit FriendChatChanged(friend_id);
        return;
    }
}

void UserMgr::FailPendingMessages()
{
    for (const auto &info : _friend_list) {
        bool changed = false;
        for (const auto &message : info->_chat_thread->_chat_msgs) {
            if (message->_status != ChatMessageStatus::Sending)
                continue;
            message->_status = ChatMessageStatus::Failed;
            changed = true;
        }
        if (changed)
            emit FriendChatChanged(info->_uid);
    }
}

qint64 UserMgr::ThreadId(int friend_id) const
{
    auto it = _friend_map.constFind(friend_id);
    return it == _friend_map.cend() ? 0 : it.value()->_chat_thread->_thread_id;
}

int UserMgr::FriendIdByThread(qint64 thread_id) const
{
    for (const auto &info : _friend_list)
        if (info->_chat_thread->_thread_id == thread_id)
            return info->_uid;
    return 0;
}

qint64 UserMgr::LastMessageId(int friend_id) const
{
    auto it = _friend_map.constFind(friend_id);
    if (it == _friend_map.cend() || it.value()->_chat_thread->_chat_msgs.empty())
        return 0;
    qint64 lastId = 0;
    for (const auto &message : it.value()->_chat_thread->_chat_msgs)
        lastId = qMax(lastId, message->_message_id);
    return lastId;
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
    const bool hadIcon = !GetIcon().isEmpty();
    emit ApplyListAboutToChange();
    emit FriendListAboutToChange();
    _user_info.reset();
    _localIcon.clear();
    _apply_list.clear();
    _friend_list.clear();
    _friend_map.clear();
    _pending_private_chats.clear();

    emit ApplyListChanged();
    emit FriendListChanged();
    if (hadIcon)
        emit UserIconChanged();
}
