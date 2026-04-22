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

//功能的id
enum ReqId{
    ID_GET_VERIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002, //注册用户
};

//模块
enum Modules{
    REGISTERMOD = 0, //注册模块
    };

//错误码
enum ErrorCodes{
    SUCCESS = 0, //成功
    ERR_JSON = 1, //json 解析失败
    ERR_NETWORK = 2, //网络错误
};

#endif // GLOBAL_H
