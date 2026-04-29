#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    //点击注册按钮触发切换信号
    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    //设置忘记密码标签的状态样式
    ui->forget_label->SetState("normal", "hover", "", "selected", "selected_hover", "");
    //连接忘记密码标签的点击信号到对应的槽函数
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
}

LoginDialog::~LoginDialog()
{
    qDebug() << "LoginDialog destruct";
    delete ui;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}
