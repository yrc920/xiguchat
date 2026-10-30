#ifndef RESETBRIDGE_H
#define RESETBRIDGE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "pagebridge.h"

/**
 * @brief 重置密码页桥接对象
 */
class ResetBridge : public PageBridge
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString code READ code WRITE setCode NOTIFY codeChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool codeBusy READ codeBusy NOTIFY codeBusyChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString tipText READ tipText NOTIFY tipTextChanged)
    Q_PROPERTY(bool tipOk READ tipOk NOTIFY tipOkChanged)
    Q_PROPERTY(bool done READ done NOTIFY doneChanged)
    Q_PROPERTY(int countdown READ countdown NOTIFY countdownChanged)

public:
    explicit ResetBridge(QObject *parent = nullptr);

    QString user() const { return _user; }
    void setUser(const QString &value);
    QString email() const { return _email; }
    void setEmail(const QString &value);
    QString code() const { return _code; }
    void setCode(const QString &value);
    QString password() const { return _password; }
    void setPassword(const QString &value);

    bool codeBusy() const { return _codeBusy; }
    bool busy() const { return _busy; }
    QString tipText() const { return _tipText; }
    bool tipOk() const { return _tipOk; }
    bool done() const { return _done; }
    int countdown() const { return _countdown; }

    Q_INVOKABLE void sendCode();
    Q_INVOKABLE void submit();

signals:
    void userChanged();
    void emailChanged();
    void codeChanged();
    void passwordChanged();
    void busyChanged();
    void codeBusyChanged();
    void tipTextChanged();
    void tipOkChanged();
    void doneChanged();
    void countdownChanged();

private:
    bool navigationBlocked() const override { return _busy || _codeBusy; }
    bool checkEmail();
    void setTip(const QString &text, bool ok);
    void setBusy(bool busy);

    QString _user;
    QString _email;
    QString _code;
    QString _password;
    QString _tipText;
    bool _tipOk = false;
    bool _busy = false;
    bool _codeBusy = false;
    bool _done = false;
    int _countdown = 0;
    QTimer _countdownTimer;
};

#endif // RESETBRIDGE_H
