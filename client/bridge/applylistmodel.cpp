#include "applylistmodel.h"

#include "usermgr.h"
#include "userdata.h"

ApplyListModel::ApplyListModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(UserMgr::GetInstance().get(), &UserMgr::ApplyListAboutToChange,
            this, [this] { beginResetModel(); });
    connect(UserMgr::GetInstance().get(), &UserMgr::ApplyListChanged,
            this, &ApplyListModel::onApplyListChanged);
}

// 行数取 UserMgr 的申请条数
int ApplyListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return UserMgr::GetInstance()->ApplyCount();
}

// 按角色取字段；status 为 0 表示还显示「添加」按钮
QVariant ApplyListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= rowCount())
        return {};
    auto info = UserMgr::GetInstance()->ApplyAt(index.row());
    if (!info)
        return {};

    switch (role) {
    case UidRole: return info->_uid;
    case NameRole: return info->_name;
    case NickRole: return info->_nick;
    case IconRole: return info->_icon;
    case DescRole: return info->_desc;
    case StatusRole: return info->_status;
    default: return {};
    }
}

// 暴露给 QML 的角色名
QHash<int, QByteArray> ApplyListModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {IconRole, "icon"},
        {DescRole, "desc"},
        {StatusRole, "status"},
    };
}

// 未处理的申请数量，侧栏红点用它
int ApplyListModel::pendingCount() const
{
    int pending = 0;
    const int total = UserMgr::GetInstance()->ApplyCount();
    for (int row = 0; row < total; ++row) {
        auto info = UserMgr::GetInstance()->ApplyAt(row);
        if (info && info->_status == 0)
            ++pending;
    }
    return pending;
}

QVariantMap ApplyListModel::get(int row) const
{
    const QStringList names{"uid", "name", "nick", "icon", "desc", "status"};
    QVariantMap result;
    for (int i = 0; i < names.size(); ++i)
        result.insert(names.at(i), data(index(row, 0), UidRole + i));
    return result;
}

// 申请列表变化（新增、状态变更）：整表重置
void ApplyListModel::onApplyListChanged()
{
    endResetModel();
    emit countChanged();
}
