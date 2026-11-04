#include "messagemodel.h"

#include "usermgr.h"
#include "userdata.h"

MessageModel::MessageModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendListAboutToChange,
            this, [this] { beginResetModel(); });
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendListChanged, this, [this] {
        auto info = UserMgr::GetInstance()->GetFriendById(_uid);
        if (info)
            syncCache(info->_chat_thread->_chat_msgs);
        else {
            _cachedCount = 0;
            _cachedIds.clear();
            _cachedStatus.clear();
        }
        endResetModel();
        emit countChanged();
    });
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendChatChanged,
            this, &MessageModel::onFriendChatChanged);
    connect(UserMgr::GetInstance().get(), &UserMgr::UserIconChanged, this, [this] {
        auto *mgr = UserMgr::GetInstance().get();
        const auto info = mgr->GetFriendById(_uid);
        if (!info)
            return;
        const int count = qMin(_cachedCount, static_cast<int>(info->_chat_thread->_chat_msgs.size()));
        for (int row = 0; row < count; ++row) {
            if (info->_chat_thread->_chat_msgs.at(row)->_from_uid == mgr->GetUid())
                emit dataChanged(index(row, 0), index(row, 0), {AvatarRole});
        }
    });
}

// 切换当前会话；uid 为 0 时清空列表
void MessageModel::setFriend(int uid)
{
    beginResetModel();
    _uid = uid;
    auto friend_info = UserMgr::GetInstance()->GetFriendById(uid);
    if (friend_info)
        syncCache(friend_info->_chat_thread->_chat_msgs);
    else {
        _cachedCount = 0;
        _cachedIds.clear();
        _cachedStatus.clear();
    }
    endResetModel();
    emit countChanged();
}

// 记录当前行顺序和状态，供下一次变更做前缀比较
void MessageModel::syncCache(const std::vector<std::shared_ptr<TextChatData>>& msgs)
{
    _cachedCount = static_cast<int>(msgs.size());
    _cachedIds.resize(_cachedCount);
    _cachedStatus.resize(_cachedCount);
    for (int i = 0; i < _cachedCount; ++i) {
        if (!msgs[i]) {
            _cachedIds[i].clear();
            _cachedStatus[i] = 0;
            continue;
        }
        _cachedIds[i] = msgs[i]->_msg_id;
        _cachedStatus[i] = static_cast<int>(msgs[i]->_status);
    }
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
        || index.row() >= static_cast<int>(friend_info->_chat_thread->_chat_msgs.size())) {
        return {};
    }

    const auto &msg = friend_info->_chat_thread->_chat_msgs.at(index.row());
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
            return role == AvatarRole ? UserMgr::GetInstance()->GetIcon() : self->_name;
        return role == AvatarRole ? friend_info->_icon : friend_info->_name;
    }
    case StatusRole:
        return static_cast<int>(msg->_status);
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
        {StatusRole, "status"},
    };
}

// 当前会话消息变化：顺序不变时按增量插入或按行刷新，只有重排/缩表才 reset
void MessageModel::onFriendChatChanged(int uid)
{
    if (uid != _uid || _uid == 0)
        return;

    auto friend_info = UserMgr::GetInstance()->GetFriendById(uid);
    if (!friend_info) {
        if (_cachedCount > 0) {
            beginResetModel();
            _cachedCount = 0;
            _cachedIds.clear();
            _cachedStatus.clear();
            endResetModel();
            emit countChanged();
        }
        return;
    }

    const auto &msgs = friend_info->_chat_thread->_chat_msgs;
    const int newCount = static_cast<int>(msgs.size());

    if (newCount == _cachedCount) {
        bool sameOrder = true;
        for (int i = 0; i < newCount; ++i) {
            if (!msgs[i] || msgs[i]->_msg_id != _cachedIds[i]) {
                sameOrder = false;
                break;
            }
        }
        if (!sameOrder) {
            beginResetModel();
            syncCache(msgs);
            endResetModel();
            emit countChanged();
            return;
        }
        // 行数与顺序都没变，只可能是状态/内容更新；按行刷新，不重建列表
        if (newCount > 0) {
            syncCache(msgs);
            emit dataChanged(index(0, 0), index(newCount - 1, 0));
        }
        return;
    }

    if (newCount > _cachedCount) {
        bool prefix = true;
        for (int i = 0; i < _cachedCount; ++i) {
            if (!msgs[i] || msgs[i]->_msg_id != _cachedIds[i]) {
                prefix = false;
                break;
            }
        }
        if (prefix) {
            beginInsertRows(QModelIndex(), _cachedCount, newCount - 1);
            syncCache(msgs);
            endInsertRows();
            emit countChanged();
            return;
        }
    }

    // 数量减少，或待确认消息拿到 message_id 后改变了位置
    beginResetModel();
    syncCache(msgs);
    endResetModel();
    emit countChanged();
}
