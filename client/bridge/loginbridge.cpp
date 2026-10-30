#include "loginbridge.h"

#include <QRegularExpression>

#include "authservice.h"
#include "sessionservice.h"

LoginBridge::LoginBridge(QObject *parent) : PageBridge(parent)
{
    auto *auth = AuthService::GetInstance().get();
    auto *session = SessionService::GetInstance().get();

    connect(auth, &AuthService::loginSucceeded, this,
            [this](const ServerInfo &info, const QString &) {
                // 账号校验通过，进入长连接阶段，按钮保持禁用直到结果回来
                setBusyText(tr("正在连接聊天服务…"));
                SessionService::GetInstance().get()->connectToChat(info);
            });

    connect(auth, &AuthService::loginFailed, this, [this](const QString &message) {
        setBusy(false);
        setError(message);
    });

    connect(session, &SessionService::connectFailed, this, [this](const QString &message) {
        setBusy(false);
        setError(message);
    });

    connect(session, &SessionService::sessionReady, this, [this] {
        setBusy(false);
        setBusyText(QString());
    });
}

// 写入 Email，值有变化才发信号，避免 QML 绑定重复求值
void LoginBridge::setEmail(const QString &value)
{
    if (_email == value)
        return;
    _email = value;
    emit emailChanged();
    clearError();
}

// 写入 Password，值有变化才发信号，避免 QML 绑定重复求值
void LoginBridge::setPassword(const QString &value)
{
    if (_password == value)
        return;
    _password = value;
    emit passwordChanged();
    clearError();
}

// 校验通过后发起登录；请求期间按钮保持禁用，直到会话就绪或失败
void LoginBridge::login()
{
    if (_busy)
        return;

    clearError();
    if (!checkEmail() || !checkPassword())
        return;

    setBusy(true);
    setBusyText(tr("正在登录…"));
    AuthService::GetInstance()->login(_email, _password);
}

// 切换密码明文 / 密文显示
void LoginBridge::togglePasswordVisible()
{
    _passwordVisible = !_passwordVisible;
    emit passwordVisibleChanged();
}

// 清掉错误提示（用户重新输入时调用）
void LoginBridge::clearError()
{
    if (_errorText.isEmpty())
        return;
    _errorText.clear();
    emit errorTextChanged();
}

// 邮箱格式校验
bool LoginBridge::checkEmail()
{
    static const QRegularExpression regex(QStringLiteral(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)"));
    if (!regex.match(_email).hasMatch()) {
        setError(tr("邮箱地址不正确"));
        return false;
    }
    return true;
}

// 密码长度与字符集校验
bool LoginBridge::checkPassword()
{
    if (_password.length() < 6 || _password.length() > 15) {
        setError(tr("密码长度应为 6~15"));
        return false;
    }

    static const QRegularExpression regex(QStringLiteral("^[a-zA-Z0-9!@#$%^&*.]{6,15}$"));
    if (!regex.match(_password).hasMatch()) {
        setError(tr("密码不能包含非法字符"));
        return false;
    }
    return true;
}

// 请求中标记：为真时按钮禁用并显示 busyText
void LoginBridge::setBusy(bool busy)
{
    if (_busy == busy)
        return;
    _busy = busy;
    emit busyChanged();
}

// 按钮上的过程文案（正在登录 / 正在连接聊天服务）
void LoginBridge::setBusyText(const QString &text)
{
    if (_busyText == text)
        return;
    _busyText = text;
    emit busyTextChanged();
}

// 错误文案，界面用淡入展示
void LoginBridge::setError(const QString &text)
{
    if (_errorText == text)
        return;
    _errorText = text;
    emit errorTextChanged();
}
