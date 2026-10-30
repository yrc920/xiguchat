#ifndef SESSIONSERVICE_H
#define SESSIONSERVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QJsonObject>
#include "global.h"
#include "singleton.h"

/******************************************************************************
 * @file       sessionservice.h
 * @brief      会话服务：建立长连接、完成聊天服务器登录、持有当前会话信息
 *
 * HTTP 登录成功后由 AuthService 交出 ServerInfo，这里负责连 TCP 并发登录帧。
 * 登录回包与通知由这里写入 UserMgr，会话就绪后才通知应用外壳进入聊天页。
 *****************************************************************************/

class SessionService : public QObject, public Singleton<SessionService>
{
    Q_OBJECT

public:
    friend class Singleton<SessionService>;
    ~SessionService();

    // 连接聊天服务器并登录；同一时刻只保留最后一次请求
    void connectToChat(const ServerInfo &info);
    // 主动断开（被踢或异常下线时调用）
    void closeSession();

    bool isReady() const { return _ready; }
    ServerInfo session() const { return _current; }

signals:
    void connecting();                       // 开始连接，界面可显示进度
    void connectFailed(const QString &msg);  // 连接层面失败（超时 / 未连上）
    void sessionReady();                     // 长连接就绪，可以进入聊天界面

private:
    SessionService();

    void onTcpConnected(bool ok);
    void onChatLoginFailed(int code);
    void onChatLoggedIn(const QJsonObject &data);
    void failAndReset(const QString &msg);

    ServerInfo _current{};
    ServerInfo _pending{};   // 等待连接的会话信息
    bool _ready;
    bool _connecting = false;
    QTimer _timeout;       // 连接 + 登录的整体超时，避免界面无限等待
    QTimer _heartbeat;     // 会话建立后的保活
};

#endif // SESSIONSERVICE_H
