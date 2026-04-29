#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
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
    Ui::LoginDialog *ui;

private slots:
    void slot_forget_pwd(); //忘记密码的槽函数

signals:
    void switchRegister(); //切换到注册界面的信号
    void switchReset(); //切换到重置密码界面的信号
};

#endif // LOGINDIALOG_H
