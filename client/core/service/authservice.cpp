#include "authservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QDebug>

#include "httpmgr.h"

namespace {

// 与 GateServer 的 ErrorCodes 一一对应（server/services/GateServer/include/const.h）
enum class Flow { Login, Register, Reset };

QString codeText(int code, Flow flow)
{
    switch (code) {
    case 1001: return QStringLiteral("请求数据不完整，请重试");
    case 1002: return QStringLiteral("服务器繁忙，请稍后再试");
    case 1003: return QStringLiteral("验证码已过期，请重新获取");
    case 1004: return QStringLiteral("验证码不正确");
    case 1005: return QStringLiteral("该邮箱或用户名已被注册");
    case 1006:
        // 注册流程里表示「两次输入的密码不一致」
        return flow == Flow::Register ? QStringLiteral("两次输入的密码不一致")
                                      : QStringLiteral("密码不正确");
    case 1007: return QStringLiteral("用户名与邮箱不匹配");
    case 1008: return QStringLiteral("密码更新失败，请稍后再试");
    case 1009: return QStringLiteral("邮箱或密码不正确");
    default: break;
    }
    return QStringLiteral("操作失败（错误码 %1）").arg(code);
}

// 解析回包：成功时给出对象，失败时给出可直接展示的文案
bool parse(const QString &res, Flow flow, QJsonObject *obj, QString *message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        *message = QStringLiteral("服务器返回的数据无法解析");
        return false;
    }
    *obj = doc.object();
    const int code = obj->value(QStringLiteral("error")).toInt(-1);
    if (code != 0) {
        *message = codeText(code, flow);
        return false;
    }
    return true;
}

} // namespace

AuthService::AuthService()
{
    auto *http = HttpMgr::GetInstance().get();
    connect(http, &HttpMgr::sig_login_mod_finish, this, &AuthService::onLoginResponse);
    connect(http, &HttpMgr::sig_reg_mod_finish, this, &AuthService::onRegResponse);
    connect(http, &HttpMgr::sig_reset_mod_finish, this, &AuthService::onResetResponse);
}

AuthService::~AuthService()
{
}

// 发起登录：密码按协议做 XOR 变形，成功后从回包取 uid / host / port / token
void AuthService::login(const QString &email, const QString &password)
{
    QJsonObject body;
    body["email"] = email;
    body["passwd"] = xorString(password); //与网关约定：密码做可逆混淆后再传输

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_login"), body,
        ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

// 请求验证码；module 区分注册与重置，两条流程的回包各走各的槽
void AuthService::sendVerifyCode(const QString &email, Modules module)
{
    QJsonObject body;
    body["email"] = email;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/get_verifycode"), body,
        ReqId::ID_GET_VERIFY_CODE, module);
}

// 提交注册：用户名 + 邮箱 + 密码 + 确认密码 + 验证码
void AuthService::registerUser(const QString &user, const QString &email,
                               const QString &password, const QString &confirm,
                               const QString &code)
{
    QJsonObject body;
    body["user"] = user;
    body["email"] = email;
    body["passwd"] = xorString(password);
    body["confirm"] = xorString(confirm);
    body["verifycode"] = code;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_register"), body,
        ReqId::ID_REG_USER, Modules::REGISTERMOD);
}

// 提交重置密码：用户名 + 邮箱 + 新密码 + 验证码
void AuthService::resetPassword(const QString &user, const QString &email,
                                const QString &password, const QString &code)
{
    QJsonObject body;
    body["user"] = user;
    body["email"] = email;
    body["passwd"] = xorString(password);
    body["verifycode"] = code;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/reset_pwd"), body,
        ReqId::ID_RESET_PWD, Modules::RESETMOD);
}

// 登录回包：错误码翻成文案，成功则把服务器信息交给会话层
void AuthService::onLoginResponse(ReqId id, QString res, ErrorCodes err)
{
    Q_UNUSED(id)

    if (err != ErrorCodes::SUCCESS) {
        emit loginFailed(QStringLiteral("网络连接失败，请检查服务器地址"));
        return;
    }

    QJsonObject obj;
    QString message;
    if (!parse(res, Flow::Login, &obj, &message)) {
        emit loginFailed(message);
        return;
    }

    ServerInfo info;
    info.Uid = obj["uid"].toInt();
    info.Host = obj["host"].toString();
    info.Port = obj["port"].toString();
    info.Token = obj["token"].toString();

    qDebug() << "[auth] login ok, uid =" << info.Uid
             << "host =" << info.Host << "port =" << info.Port;

    emit loginSucceeded(info, obj["email"].toString());
}

// 注册回包：成功带上 uid，失败给出可展示的文案
void AuthService::onRegResponse(ReqId id, QString res, ErrorCodes err)
{
    if (id == ReqId::ID_GET_VERIFY_CODE) {
        if (err != ErrorCodes::SUCCESS) {
            emit verifyCodeFailed(QStringLiteral("网络连接失败，请检查服务器地址"),
                                  Modules::REGISTERMOD);
            return;
        }
        QJsonObject obj;
        QString message;
        if (!parse(res, Flow::Register, &obj, &message)) {
            emit verifyCodeFailed(message, Modules::REGISTERMOD);
            return;
        }
        emit verifyCodeSent(obj["email"].toString(), Modules::REGISTERMOD);
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        emit registerFailed(QStringLiteral("网络连接失败，请检查服务器地址"));
        return;
    }
    QJsonObject obj;
    QString message;
    if (!parse(res, Flow::Register, &obj, &message)) {
        emit registerFailed(message);
        return;
    }
    emit registered(obj["email"].toString(), obj["uid"].toInt());
}

// 重置密码回包：成功提示改密完成，失败给出原因
void AuthService::onResetResponse(ReqId id, QString res, ErrorCodes err)
{
    if (id == ReqId::ID_GET_VERIFY_CODE) {
        if (err != ErrorCodes::SUCCESS) {
            emit verifyCodeFailed(QStringLiteral("网络连接失败，请检查服务器地址"),
                                  Modules::RESETMOD);
            return;
        }
        QJsonObject obj;
        QString message;
        if (!parse(res, Flow::Reset, &obj, &message)) {
            emit verifyCodeFailed(message, Modules::RESETMOD);
            return;
        }
        emit verifyCodeSent(obj["email"].toString(), Modules::RESETMOD);
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        emit resetFailed(QStringLiteral("网络连接失败，请检查服务器地址"));
        return;
    }
    QJsonObject obj;
    QString message;
    if (!parse(res, Flow::Reset, &obj, &message)) {
        emit resetFailed(message);
        return;
    }
    emit passwordReset(obj["email"].toString());
}
