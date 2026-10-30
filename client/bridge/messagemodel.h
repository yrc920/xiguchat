#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

/**
 * @brief 当前会话的消息模型
 *
 * 行数据直接读 UserMgr 里该好友的 _chat_msgs，不复制一份。
 * 切换会话时整表重置；收到/发出新消息时按增量插入行。
 */
class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        TextRole = Qt::UserRole + 1,
        SelfRole,
        FromUidRole,
        ImageSourceRole,
        ImageWidthRole,
        ImageHeightRole,
        AvatarRole,
        NameRole,
    };

    explicit MessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 切换到某个好友的会话，uid 为 0 表示清空
    void setFriend(int uid);

signals:
    void countChanged();

private:
    void onFriendChatChanged(int uid);

    int _uid = 0;
    int _cachedCount = 0;
};

#endif // MESSAGEMODEL_H
