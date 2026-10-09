#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"
/******************************************************************************
 *
 * @file       logindialog.h
 * @brief      登录界面
 *
 * @author     西故
 * @date       2026/04/19
 * @history
 *****************************************************************************/

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT //使得LoginDialog可以使用Qt的信号和槽机制

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    void initHttpHandlers(); //初始化http响应处理函数
    void initHead(); //初始化头像
    void showTip(QString str,bool b_ok); //显示提示信息

    void AddTipErr(TipErr te,QString tips); //添加错误提示信息
    void DelTipErr(TipErr te); //删除错误提示信息
    bool checkEmailValid(); //检查邮箱是否合法
    bool checkPwdValid(); //检查密码是否合法
    bool enableBtn(bool); //设置按钮可用状态

    Ui::LoginDialog *ui;
    //请求id和对应的处理函数的映射表, 当http请求完成时, 根据请求id找到对应的处理函数进行处理
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs; //错误类型和对应的提示信息的映射表
    int _uid; //用户ID
    QString _token; //用户令牌

private slots:
    void slot_forget_pwd(); //忘记密码的槽函数
    void on_login_btn_clicked(); //登录按钮的槽函数
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err); //登录模块http请求完成的槽函数
    void slot_tcp_con_finish(bool bsuccess); //tcp连接完成的槽函数，参数表示连接是否成功
    void slot_login_failed(int err); //登录失败的槽函数，参数为错误码

signals:
    void switchRegister(); //切换到注册界面的信号
    void switchReset(); //切换到重置密码界面的信号
    void sig_connect_tcp(ServerInfo); //连接聊天服务器的信号，参数是服务器信息结构体
};

#endif // LOGINDIALOG_H
