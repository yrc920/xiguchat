#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>
#include "global.h"
/******************************************************************************
 *
 * @file       resetdialog.h
 * @brief      重置密码界面
 *
 * @author     西故
 * @date       2026/04/28
 * @history
 *****************************************************************************/

namespace Ui {
class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT //使得ResetDialog可以使用Qt的信号和槽机制

public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();

private:
    void initHandlers(); //初始化http响应处理函数
    void showTip(QString str,bool b_ok); //显示提示信息

    void AddTipErr(TipErr te,QString tips); //添加错误提示信息
    void DelTipErr(TipErr te); //删除错误提示信息
    bool checkUserValid(); //检查用户名是否合法
    bool checkEmailValid(); //检查邮箱是否合法
    bool checkPassValid(); //检查密码是否合法
    bool checkVerifyValid(); //检查验证码是否合法

    Ui::ResetDialog *ui;
    //请求id和对应的处理函数的映射表, 当http请求完成时, 根据请求id找到对应的处理函数进行处理
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs; //错误类型和对应的提示信息的映射表

private slots:
    void on_verify_btn_clicked(); //获取验证码按钮的槽函数
    void slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err); //重置模块http请求完成的槽函数
    void on_sure_btn_clicked(); //重置按钮的槽函数
    void on_return_btn_clicked(); //返回登录按钮的槽函数

signals:
    void switchLogin(); //切换到登录界面的信号
};

#endif // RESETDIALOG_H
