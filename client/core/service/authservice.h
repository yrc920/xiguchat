#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QObject>
#include <QString>
#include "global.h"
#include "singleton.h"

/******************************************************************************
 * @file       authservice.h
 * @brief      账号鉴权服务：验证码 / 注册 / 重置密码 / 登录
 *
 * 只负责协议：构造请求、解析响应、把结果翻译成界面能直接展示的文案。
 * 不持有任何界面状态，调用方通过信号拿到结果。
 *****************************************************************************/

class AuthService : public QObject, public Singleton<AuthService>
{
    Q_OBJECT

public:
    friend class Singleton<AuthService>;
    ~AuthService();

    void login(const QString &email, const QString &password);
    // module 决定验证码走注册还是重置流程（两者的回包信号不同）
    void sendVerifyCode(const QString &email, Modules module);
    void registerUser(const QString &user, const QString &email, const QString &password,
                      const QString &confirm, const QString &code);
    void resetPassword(const QString &user, const QString &email, const QString &password,
                       const QString &code);

signals:
    void loginSucceeded(const ServerInfo &info, const QString &email);
    void loginFailed(const QString &message);

    void verifyCodeSent(const QString &email, Modules module);
    void verifyCodeFailed(const QString &message, Modules module);

    void registered(const QString &email, int uid);
    void registerFailed(const QString &message);

    void passwordReset(const QString &email);
    void resetFailed(const QString &message);

private:
    AuthService();
    void onLoginResponse(ReqId id, QString res, ErrorCodes err);
    void onRegResponse(ReqId id, QString res, ErrorCodes err);
    void onResetResponse(ReqId id, QString res, ErrorCodes err);
};

#endif // AUTHSERVICE_H
