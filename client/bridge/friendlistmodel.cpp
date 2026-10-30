#include "friendlistmodel.h"

#include "usermgr.h"
#include "userdata.h"

FriendListModel::FriendListModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(UserMgr::GetInstance().get(), &UserMgr::FriendListAboutToChange,
            this, [this] { beginResetModel(); });
    auto *mgr = UserMgr::GetInstance().get();
    connect(mgr, &UserMgr::FriendListChanged, this, &FriendListModel::onFriendListChanged);
    connect(mgr, &UserMgr::FriendChatChanged, this, [this](int uid) {
        const int row = indexOfUid(uid);
        if (row < 0)
            return;
        // 只刷新这一行的「最后一条消息」，不整表重置，避免列表闪烁
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {LastTextRole});
    });
}

// 行数直接取 UserMgr 的好友数量，模型自己不存副本
int FriendListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return UserMgr::GetInstance()->FriendCount();
}

// 按角色取字段；lastText 为空时界面显示「还没有消息」
QVariant FriendListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= rowCount())
        return {};
    auto friend_info = UserMgr::GetInstance()->FriendAt(index.row());
    if (!friend_info)
        return {};

    switch (role) {
    case UidRole: return friend_info->_uid;
    case NameRole: return friend_info->_name;
    case NickRole: return friend_info->_nick;
    case IconRole: return friend_info->_icon;
    case DescRole: return friend_info->_desc;
    case LastTextRole:
        if (!friend_info->_chat_msgs.empty()) {
            const auto &last = friend_info->_chat_msgs.back();
            return last->_imageSource.isEmpty() ? last->_msg_content : QStringLiteral("[图片]");
        }
        return friend_info->_last_msg;
    case InitialRole: {
        const QString name = friend_info->_name.isEmpty() ? friend_info->_nick
                                                          : friend_info->_name;
        return name.isEmpty() ? QStringLiteral("#") : name.left(1).toUpper();
    }
    default:
        return {};
    }
}

// 暴露给 QML 的角色名，delegate 用 model.xxx 读取
QHash<int, QByteArray> FriendListModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {IconRole, "icon"},
        {DescRole, "desc"},
        {LastTextRole, "lastText"},
        {InitialRole, "initial"},
    };
}

// 按 uid 找行号，用于消息更新后只刷新那一行
int FriendListModel::indexOfUid(int uid) const
{
    const int total = UserMgr::GetInstance()->FriendCount();
    for (int row = 0; row < total; ++row) {
        auto friend_info = UserMgr::GetInstance()->FriendAt(row);
        if (friend_info && friend_info->_uid == uid)
            return row;
    }
    return -1;
}

QVariantMap FriendListModel::get(int row) const
{
    QStringList names{"uid", "name", "nick", "icon", "desc", "lastText", "initial"};
    QVariantMap result;
    for (int i = 0; i < names.size(); ++i) {
        result.insert(names.at(i), data(index(row, 0), UidRole + i));
    }
    return result;
}

// 好友列表整体变化（登录、通过验证）：整表重置
void FriendListModel::onFriendListChanged()
{
    endResetModel();
    emit countChanged();
}
