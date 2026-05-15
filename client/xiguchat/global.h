#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"
#include <memory>
#include <iostream>
#include <mutex>
#include <QDebug>
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
    ID_SEARCH_USER_REQ = 1007, //用户搜索请求
    ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,  //添加好友申请
    ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ  = 1017,  //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP  = 1018,  //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
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
    ERR_JSON = 1, //json解析失败
    ERR_NETWORK = 2, //网络错误
};

//点击标签的状态
enum ClickLbState{
    Normal = 0, //正常状态
    Selected = 1 //选中状态
};

extern QString gate_url_prefix; //网关服务器的URL前缀,从配置文件中读取

//服务器信息结构体
struct ServerInfo{
    QString Host; //服务器地址
    QString Port; //服务器端口
    QString Token; //用户令牌
    int Uid; //用户ID
};

//聊天界面几种模式
enum ChatUIMode{
    SearchMode, //搜索模式
    ChatMode, //聊天模式
    ContactMode //联系人模式
};

//聊天项的角色
enum class ChatRole
{
    Self, //自己
    Other //别人
};

//消息信息结构体, 用于存储用户输入的文本、图片和文件等消息内容
struct MsgInfo{
    QString msgFlag; //"text, image, file"
    QString content; //表示文件和图像的url, 文本信息
    QPixmap pixmap; //文件和图片的缩略图
};

//自定义QListWidgetItem的几种类型
enum ListItemType{
    CHAT_USER_ITEM, //聊天用户
    CONTACT_USER_ITEM, //联系人用户
    SEARCH_USER_ITEM, //搜索到的用户
    ADD_USER_TIP_ITEM, //提示添加用户
    INVALID_ITEM,  //不可点击条目
    GROUP_TIP_ITEM, //分组提示条目
    LINK_ITEM, //分割线
    APPLY_FRIEND_ITEM, //好友申请
};

const int MIN_APPLY_LABEL_ED_LEN = 50; //申请好友标签输入框最低长度
const QString add_prefix = "添加标签 ";
const int tip_offset = 5; //标签之间的间距

const std::vector<QString>  strs ={"hello world !",
                                   "nice to meet u",
                                   "New year，new life",
                                   "You have to love yourself",
                                   "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

#endif // GLOBAL_H
