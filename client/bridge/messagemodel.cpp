#include "messagemodel.h"

#include "usermgr.h"
#include "userdata.h"

MessageModel::MessageModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendListAboutToChange,
            this, [this] { beginResetModel(); });
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendListChanged, this, [this] {
        auto info = UserMgr::GetInstance()->GetFriendById(_uid);
        _cachedCount = info ? static_cast<int>(info->_chat_msgs.size()) : 0;
        endResetModel();
        emit countChanged();
    });
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendChatChanged,
            this, &MessageModel::onFriendChatChanged);
}

// 切换当前会话；uid 为 0 时清空列表
void MessageModel::setFriend(int uid)
{
    beginResetModel();
    _uid = uid;
    auto friend_info = UserMgr::GetInstance()->GetFriendById(uid);
    _cachedCount = friend_info ? static_cast<int>(friend_info->_chat_msgs.size()) : 0;
    endResetModel();
    emit countChanged();
}

// 行数取当前好友的消息条数
int MessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || _uid == 0)
        return 0;
    return _cachedCount;
}

// 按角色取字段；isSelf 决定气泡左右
QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= rowCount())
        return {};
    auto friend_info = UserMgr::GetInstance()->GetFriendById(_uid);
    if (!friend_info || index.row() < 0
        || index.row() >= static_cast<int>(friend_info->_chat_msgs.size())) {
        return {};
    }

    const auto &msg = friend_info->_chat_msgs.at(index.row());
    switch (role) {
    case TextRole: return msg->_msg_content;
    case SelfRole: return msg->_from_uid == UserMgr::GetInstance()->GetUid();
    case FromUidRole: return msg->_from_uid;
    case ImageSourceRole: return msg->_imageSource;
    case ImageWidthRole: return msg->_imageWidth;
    case ImageHeightRole: return msg->_imageHeight;
    case AvatarRole:
    case NameRole: {
        auto self = UserMgr::GetInstance()->GetUserInfo();
        if (self && msg->_from_uid == self->_uid)
            return role == AvatarRole ? self->_icon : self->_name;
        return role == AvatarRole ? friend_info->_icon : friend_info->_name;
    }
    default: return {};
    }
}

// 暴露给 QML 的角色名
QHash<int, QByteArray> MessageModel::roleNames() const
{
    return {
        {TextRole, "text"},
        {SelfRole, "isSelf"},
        {FromUidRole, "fromUid"},
        {ImageSourceRole, "imageSource"},
        {ImageWidthRole, "imageWidth"},
        {ImageHeightRole, "imageHeight"},
        {AvatarRole, "avatar"},
        {NameRole, "name"},
    };
}

// 当前会话来了新消息：增量插入，不整表重置
void MessageModel::onFriendChatChanged(int uid)
{
    if (uid != _uid || _uid == 0)
        return;

    auto friend_info = UserMgr::GetInstance()->GetFriendById(uid);
    const int newCount = friend_info ? static_cast<int>(friend_info->_chat_msgs.size()) : 0;

    if (newCount > _cachedCount) {
        // 追加：只插入新增的行，已有气泡不重建
        beginInsertRows(QModelIndex(), _cachedCount, newCount - 1);
        _cachedCount = newCount;
        endInsertRows();
        emit countChanged();
    } else if (newCount < _cachedCount) {
        beginResetModel();
        _cachedCount = newCount;
        endResetModel();
        emit countChanged();
    }
}
