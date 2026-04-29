#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LoginDialog *_login_dlg; //登录界面
    RegisterDialog *_reg_dlg; //注册界面
    ResetDialog *_reset_dlg; //重置密码界面

public slots:
    void SlotSwitchReg(); //切换到注册界面槽函数
    void SlotSwitchLogin(); //切换到登录界面槽函数
    void SlotSwitchReset(); //切换到重置密码界面槽函数
    void SlotSwitchLoginFromReset(); //从重置密码界面切换到登录界面槽函数
};
#endif // MAINWINDOW_H
