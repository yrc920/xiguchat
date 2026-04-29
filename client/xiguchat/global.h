#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"
#include <memory>
#include <iostream>
#include <mutex>
#include <QByteArray>
#include <QNetworkReply>
#include <QJsonObject>
#include <QDir>
#include <QSettings>
/******************************************************************************
 *
 * @file       global.h
 * @brief      全局变量和枚举
 *
 * @author     西故
 * @date       2026/04/20
 * @history
 *****************************************************************************/

/**
 * @brief repolish 用来刷新qss
 */
extern std::function<void(QWidget*)> repolish;

//简单的异或加密函数, 输入一个字符串, 返回加密后的字符串
extern std::function<QString(QString)> xorString;

//功能的id
enum ReqId{
    ID_GET_VERIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002, //注册用户
    ID_RESET_PWD = 1003, //重置密码
    ID_LOGIN_USER = 1004, //用户登录
    ID_CHAT_LOGIN = 1005, //登陆聊天服务器
    ID_CHAT_LOGIN_RSP = 1006, //登陆聊天服务器回包
};

//模块
enum Modules{
    REGISTERMOD = 0, //注册模块
    RESETMOD = 1, //重置密码模块
    LOGINMOD = 2, //登录模块
};

//错误提示
enum TipErr{
    TIP_SUCCESS = 0, //成功
    TIP_EMAIL_ERR = 1, //邮箱错误
    TIP_PWD_ERR = 2, //密码错误
    TIP_CONFIRM_ERR = 3, //确认密码错误
    TIP_PWD_CONFIRM = 4, //密码和确认密码不匹配
    TIP_VERIFY_ERR = 5, //验证码错误
    TIP_USER_ERR = 6 //用户名错误
};

//错误码
enum ErrorCodes{
    SUCCESS = 0, //成功
    ERR_JSON = 1, //json 解析失败
    ERR_NETWORK = 2, //网络错误
};

//点击标签的状态
enum ClickLbState{
    Normal = 0, //正常状态
    Selected = 1 //选中状态
};

extern QString gate_url_prefix; //网关服务器的URL前缀,从配置文件中读取

#endif // GLOBAL_H
