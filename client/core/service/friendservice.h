#ifndef FRIENDSERVICE_H
#define FRIENDSERVICE_H

#include <QObject>
#include <QString>
#include "singleton.h"

/**
 * @brief 好友关系服务：搜索用户、发起申请、通过申请
 *
 * 只负责协议与结果翻译，界面通过信号拿到数据。
 */
class FriendService : public QObject, public Singleton<FriendService>
{
    Q_OBJECT

public:
    friend class Singleton<FriendService>;
    ~FriendService();

    // query 纯数字按 uid 查，否则按用户名查
    bool searchUser(const QString &query);
    void applyFriend(int touid, const QString &applyName, const QString &backName);
    void authFriend(int touid, const QString &backName);

signals:
    void userFound(int uid, const QString &name, const QString &nick,
                   const QString &desc, const QString &icon, int sex);
    void searchFailed(const QString &message);

private:
    FriendService();
    bool _searching = false;
};

#endif // FRIENDSERVICE_H
