#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include <QLayout>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    _ui_status = LOGIN_UI;
    ui->setupUi(this);
    _login_dlg = new LoginDialog(this); //创建登录页面的实例
    //设置登录页面和注册页面为无边框窗口, 相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

    //连接创建聊天界面信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);

    //链接服务器踢人消息
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_notify_offline, this, &MainWindow::SlotOffline);

    //测试用
    //emit TcpMgr::GetInstance()->sig_swich_chatdlg();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this); //创建注册页面的实例
    //设置登录页面和注册页面为无边框窗口, 相当于将它们嵌入到主窗口中
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_reg_dlg); //将注册页面设置为主窗口的中心组件
    _login_dlg->hide(); //隐藏登录页面
    _reg_dlg->show(); //显示注册页面

    //连接注册页面切换到登录页面的信号到对应的槽函数
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    _ui_status = REGISTER_UI;
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this); //创建登录页面的实例
     //设置登录页面和注册页面为无边框窗口, 相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件
    _reg_dlg->hide(); //隐藏注册页面
    _login_dlg->show(); //显示登录页面

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchReset()
{
    _ui_status = RESET_UI;
    _reset_dlg = new ResetDialog(this); //创建重置密码页面的实例
     //设置登录页面和注册页面为无边框窗口, 相当于将它们嵌入到主窗口中
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
    //设置登录页面和注册页面为无边框窗口, 相当于将它们嵌入到主窗口中
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg); //将登录页面设置为主窗口的中心组件
    _reset_dlg->hide(); //隐藏重置密码页面
    _login_dlg->show(); //显示登录页面

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog(); //创建聊天页面的实例
     //设置聊天页面为无边框窗口, 相当于将它嵌入到主窗口中
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg); //将聊天页面设置为主窗口的中心组件
    _chat_dlg->show(); //显示聊天页面
    _login_dlg->hide(); //隐藏登录页面
    this->setMinimumSize(QSize(900, 650));
    //设置窗口的最大尺寸为系统允许的最大值，允许窗口被放大到全屏
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    _ui_status = CHAT_UI;
}

void MainWindow::SlotOffline(){
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
    TcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::offlineLogin(){
    if(_ui_status == LOGIN_UI)
        return;

    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _chat_dlg->hide();
    this->setMaximumSize(300,500);
    this->setMinimumSize(300,500);
    this->resize(300,500);
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_status = LOGIN_UI;
}
