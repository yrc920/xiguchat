#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _login_dlg = new LoginDialog(this); //创建登录页面的实例
    //设置登录页面和注册页面为无边框窗口,相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this); //创建注册页面的实例
    //设置登录页面和注册页面为无边框窗口,相当于将它们嵌入到主窗口中
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_reg_dlg); //将注册页面设置为主窗口的中心组件
    _login_dlg->hide(); //隐藏登录页面
    _reg_dlg->show(); //显示注册页面

    //连接注册页面切换到登录页面的信号到对应的槽函数
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this); //创建登录页面的实例
     //设置登录页面和注册页面为无边框窗口,相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件
    _reg_dlg->hide(); //隐藏注册页面
    _login_dlg->show(); //显示登录页面

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    _reset_dlg = new ResetDialog(this); //创建重置密码页面的实例
     //设置登录页面和注册页面为无边框窗口,相当于将它们嵌入到主窗口中
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg); //将重置密码页面设置为主窗口的中心组件
    _login_dlg->hide(); //隐藏登录页面
    _reset_dlg->show(); //显示重置密码页面

    //注册返回登录信号和槽函数
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLoginFromReset);
}

//从重置界面返回登录界面
void MainWindow::SlotSwitchLoginFromReset()
{
    _login_dlg = new LoginDialog(this); //创建登录页面的实例
     //设置登录页面和注册页面为无边框窗口,相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件
    _reset_dlg->hide(); //隐藏重置密码页面
    _login_dlg->show(); //显示登录页面

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}
