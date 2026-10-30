#ifndef APPLYLISTMODEL_H
#define APPLYLISTMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

/**
 * @brief 好友申请列表模型
 */
class ApplyListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY countChanged)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        NameRole,
        NickRole,
        IconRole,
        DescRole,
        StatusRole,
    };

    explicit ApplyListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 未处理的申请数，用于侧边栏红点
    int pendingCount() const;

    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    void onApplyListChanged();
};

#endif // APPLYLISTMODEL_H
