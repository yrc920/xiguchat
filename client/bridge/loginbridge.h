#ifndef LOGINBRIDGE_H
#define LOGINBRIDGE_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "pagebridge.h"

/******************************************************************************
 * @file       loginbridge.h
 * @brief      登录页与 C++ 之间的唯一边界
 *
 * QML 只声明结构：绑定 email / password 属性，点按钮调 login()。
 * 校验、请求、错误文案全部在这里，界面不出现任何协议字段。
 *****************************************************************************/

class LoginBridge : public PageBridge
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool passwordVisible READ passwordVisible NOTIFY passwordVisibleChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString busyText READ busyText NOTIFY busyTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit LoginBridge(QObject *parent = nullptr);

    QString email() const { return _email; }
    void setEmail(const QString &value);

    QString password() const { return _password; }
    void setPassword(const QString &value);

    bool passwordVisible() const { return _passwordVisible; }
    bool busy() const { return _busy; }
    QString busyText() const { return _busyText; }
    QString errorText() const { return _errorText; }

    // 提交登录：先做本地校验，再走 HTTP 登录 + 长连接登录
    Q_INVOKABLE void login();
    Q_INVOKABLE void togglePasswordVisible();
    Q_INVOKABLE void clearError();

signals:
    void emailChanged();
    void passwordChanged();
    void passwordVisibleChanged();
    void busyChanged();
    void busyTextChanged();
    void errorTextChanged();

private:
    bool navigationBlocked() const override { return _busy; }
    bool checkEmail();
    bool checkPassword();
    void setBusy(bool busy);
    void setBusyText(const QString &text);
    void setError(const QString &text);

    QString _email;
    QString _password;
    QString _busyText;
    QString _errorText;
    bool _passwordVisible = false;
    bool _busy = false;
};

#endif // LOGINBRIDGE_H
