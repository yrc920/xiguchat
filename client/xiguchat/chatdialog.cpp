#include "chatdialog.h"
#include "ui_chatdialog.h"

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press"); //设置添加好友按钮的状态样式
}

ChatDialog::~ChatDialog()
{
    delete ui;
}
