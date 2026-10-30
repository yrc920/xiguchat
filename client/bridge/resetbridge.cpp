#include "resetbridge.h"

#include <QRegularExpression>

#include "authservice.h"

namespace {
constexpr int kResendSeconds = 60;

bool validEmail(const QString &email)
{
    static const QRegularExpression regex(
        QStringLiteral(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)"));
    return regex.match(email).hasMatch();
}

bool validPassword(const QString &password)
{
    if (password.length() < 6 || password.length() > 15)
        return false;
    static const QRegularExpression regex(QStringLiteral("^[a-zA-Z0-9!@#$%^&*.]{6,15}$"));
    return regex.match(password).hasMatch();
}
} // namespace

ResetBridge::ResetBridge(QObject *parent) : PageBridge(parent)
{
    _countdownTimer.setInterval(1000);
    connect(&_countdownTimer, &QTimer::timeout, this, [this] {
        if (_countdown <= 1) {
            _countdown = 0;
            _countdownTimer.stop();
        } else {
            _countdown--;
        }
        emit countdownChanged();
    });

    auto *auth = AuthService::GetInstance().get();
    connect(auth, &AuthService::verifyCodeSent, this,
            [this](const QString &, Modules module) {
                if (module != Modules::RESETMOD || !_codeBusy)
                    return;
                _codeBusy = false;
                emit codeBusyChanged();
                setTip(QStringLiteral("验证码已发送到邮箱，注意查收"), true);
                _countdown = kResendSeconds;
                _countdownTimer.start();
                emit countdownChanged();
            });
    connect(auth, &AuthService::verifyCodeFailed, this,
            [this](const QString &message, Modules module) {
                if (module != Modules::RESETMOD || !_codeBusy)
                    return;
                _codeBusy = false;
                emit codeBusyChanged();
                setTip(message, false);
            });
    connect(auth, &AuthService::passwordReset, this, [this](const QString &) {
        setBusy(false);
        setTip(QStringLiteral("密码已重置，请用新密码登录"), true);
        _done = true;
        emit doneChanged();
        QTimer::singleShot(1800, this, [this] { go(QStringLiteral("login")); });
    });
    connect(auth, &AuthService::resetFailed, this, [this](const QString &message) {
        setBusy(false);
        setTip(message, false);
    });
}

// 写入 User，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setUser(const QString &value)
{
    if (_user == value) return;
    _user = value;
    emit userChanged();
}

// 写入 Email，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setEmail(const QString &value)
{
    if (_email == value) return;
    _email = value;
    emit emailChanged();
}

// 写入 Code，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setCode(const QString &value)
{
    if (_code == value) return;
    _code = value;
    emit codeChanged();
}

// 写入 Password，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setPassword(const QString &value)
{
    if (_password == value) return;
    _password = value;
    emit passwordChanged();
}

// 请求邮箱验证码，成功后开始 60 秒倒计时
void ResetBridge::sendCode()
{
    if (_countdown > 0 || _codeBusy || _busy || _done)
        return;
    if (!checkEmail())
        return;

    _codeBusy = true;
    emit codeBusyChanged();
    setTip(QStringLiteral("正在发送验证码…"), true);
    AuthService::GetInstance()->sendVerifyCode(_email, Modules::RESETMOD);
}

// 逐项校验后提交改密；任一校验失败就地给提示，不发请求
void ResetBridge::submit()
{
    if (_busy || _done || _codeBusy)
        return;

    if (_user.trimmed().isEmpty()) {
        setTip(QStringLiteral("用户名不能为空"), false);
        return;
    }
    if (!checkEmail())
        return;
    if (!validPassword(_password)) {
        setTip(QStringLiteral("密码长度应为 6~15 且不能包含非法字符"), false);
        return;
    }
    if (_code.trimmed().isEmpty()) {
        setTip(QStringLiteral("请填写邮箱收到的验证码"), false);
        return;
    }

    setBusy(true);
    setTip(QStringLiteral("正在提交…"), true);
    AuthService::GetInstance()->resetPassword(_user, _email, _password, _code);
}

// 邮箱格式校验
bool ResetBridge::checkEmail()
{
    if (!validEmail(_email)) {
        setTip(QStringLiteral("邮箱地址不正确"), false);
        return false;
    }
    return true;
}

// 写入 Tip，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setTip(const QString &text, bool ok)
{
    _tipText = text;
    _tipOk = ok;
    emit tipTextChanged();
    emit tipOkChanged();
}

// 写入 Busy，值有变化才发信号，避免 QML 绑定重复求值
void ResetBridge::setBusy(bool busy)
{
    if (_busy == busy) return;
    _busy = busy;
    emit busyChanged();
}
