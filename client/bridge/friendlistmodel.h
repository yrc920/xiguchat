#ifndef FRIENDLISTMODEL_H
#define FRIENDLISTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

/**
 * @brief 好友列表模型
 *
 * 会话列表与联系人列表读的是同一份好友数据，只是 delegate 显示的东西不同，
 * 因此共用一个模型，不复制两份数据。行数据直接引用 UserMgr 里的对象，
 * UserMgr 发出变更信号时模型跟着刷新。
 */
class FriendListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        NameRole,
        NickRole,
        IconRole,
        DescRole,
        LastTextRole,
        InitialRole,   // 名字首字符，供联系人列表分组
    };

    explicit FriendListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int indexOfUid(int uid) const;
    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    void onFriendListChanged();
};

#endif // FRIENDLISTMODEL_H
