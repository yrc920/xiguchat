#include "registerbridge.h"

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

RegisterBridge::RegisterBridge(QObject *parent) : PageBridge(parent)
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
                if (module != Modules::REGISTERMOD || !_codeBusy)
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
                if (module != Modules::REGISTERMOD || !_codeBusy)
                    return;
                _codeBusy = false;
                emit codeBusyChanged();
                setTip(message, false);
            });
    connect(auth, &AuthService::registered, this, [this](const QString &, int) {
        setBusy(false);
        setTip(QStringLiteral("注册成功，正在返回登录…"), true);
        _done = true;
        emit doneChanged();
        // 留一点时间让用户看到结果，再回登录页
        QTimer::singleShot(1600, this, [this] { go(QStringLiteral("login")); });
    });
    connect(auth, &AuthService::registerFailed, this, [this](const QString &message) {
        setBusy(false);
        setTip(message, false);
    });
}

// 写入 User，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setUser(const QString &value)
{
    if (_user == value) return;
    _user = value;
    emit userChanged();
}

// 写入 Email，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setEmail(const QString &value)
{
    if (_email == value) return;
    _email = value;
    emit emailChanged();
}

// 写入 Code，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setCode(const QString &value)
{
    if (_code == value) return;
    _code = value;
    emit codeChanged();
}

// 写入 Password，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setPassword(const QString &value)
{
    if (_password == value) return;
    _password = value;
    emit passwordChanged();
}

// 写入 Confirm，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setConfirm(const QString &value)
{
    if (_confirm == value) return;
    _confirm = value;
    emit confirmChanged();
}

// 请求邮箱验证码，成功后开始 60 秒倒计时
void RegisterBridge::sendCode()
{
    if (_countdown > 0 || _codeBusy || _busy || _done)
        return;
    if (!checkEmail())
        return;

    _codeBusy = true;
    emit codeBusyChanged();
    setTip(QStringLiteral("正在发送验证码…"), true);
    AuthService::GetInstance()->sendVerifyCode(_email, Modules::REGISTERMOD);
}

// 逐项校验后提交注册；任一校验失败就地给提示，不发请求
void RegisterBridge::submit()
{
    if (_busy || _done || _codeBusy)
        return;

    if (_user.trimmed().isEmpty()) {
        setTip(QStringLiteral("用户名不能为空"), false);
        return;
    }
    if (!checkEmail())
        return;
    if (!checkPassword())
        return;
    if (_confirm != _password) {
        setTip(QStringLiteral("两次输入的密码不一致"), false);
        return;
    }
    if (_code.trimmed().isEmpty()) {
        setTip(QStringLiteral("请填写邮箱收到的验证码"), false);
        return;
    }

    setBusy(true);
    setTip(QStringLiteral("正在提交…"), true);
    AuthService::GetInstance()->registerUser(_user, _email, _password, _confirm, _code);
}

// 邮箱格式校验
bool RegisterBridge::checkEmail()
{
    if (!validEmail(_email)) {
        setTip(QStringLiteral("邮箱地址不正确"), false);
        return false;
    }
    return true;
}

// 密码长度与字符集校验
bool RegisterBridge::checkPassword()
{
    if (_password.length() < 6 || _password.length() > 15) {
        setTip(QStringLiteral("密码长度应为 6~15"), false);
        return false;
    }
    if (!validPassword(_password)) {
        setTip(QStringLiteral("密码不能包含非法字符"), false);
        return false;
    }
    return true;
}

// 写入 Tip，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setTip(const QString &text, bool ok)
{
    _tipText = text;
    _tipOk = ok;
    emit tipTextChanged();
    emit tipOkChanged();
}

// 写入 Busy，值有变化才发信号，避免 QML 绑定重复求值
void RegisterBridge::setBusy(bool busy)
{
    if (_busy == busy) return;
    _busy = busy;
    emit busyChanged();
}
