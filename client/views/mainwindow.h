#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"
/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      主窗口
 *
 * @author     西故
 * @date       2026/04/19
 * @history
 *****************************************************************************/

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum UIStatus{
    LOGIN_UI,
    REGISTER_UI,
    RESET_UI,
    CHAT_UI
};

class MainWindow : public QMainWindow
{
    Q_OBJECT //使得MainWindow可以使用Qt的信号和槽机制

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void offlineLogin();

    Ui::MainWindow *ui;
    LoginDialog *_login_dlg; //登录界面
    RegisterDialog *_reg_dlg; //注册界面
    ResetDialog *_reset_dlg; //重置密码界面
    ChatDialog *_chat_dlg; //聊天界面
    UIStatus _ui_status; //当前界面状态

public slots:
    void SlotSwitchReg(); //切换到注册界面槽函数
    void SlotSwitchLogin(); //切换到登录界面槽函数
    void SlotSwitchReset(); //切换到重置密码界面槽函数
    void SlotSwitchLoginFromReset(); //从重置密码界面切换到登录界面槽函数
    void SlotSwitchChat(); //切换到聊天界面槽函数
    void SlotOffline(); //处理被踢下线的槽函数
    void SlotExcepConOffline(); //处理异常断开连接的槽函数
};
#endif // MAINWINDOW_H
