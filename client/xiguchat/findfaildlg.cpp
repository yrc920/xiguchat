#include "findfaildlg.h"
#include "ui_findfaildlg.h"
#include <QDebug>

FindFailDlg::FindFailDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindFailDlg)
{
    ui->setupUi(this);
    setWindowTitle("添加"); //设置对话框标题
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint); //隐藏对话框标题栏
    this->setObjectName("FindFailDlg");
    ui->fail_sure_btn->SetState("normal","hover","press"); //设置搜索失败对话框的确定按钮的状态样式
    this->setModal(true); //设置对话框为模态, 阻止用户与其他窗口交互, 直到对话框被关闭
}

FindFailDlg::~FindFailDlg()
{
    qDebug()<< "Find FailDlg destruct";
    delete ui;
}

void FindFailDlg::on_fail_sure_btn_clicked()
{
    this->hide(); //隐藏搜索失败对话框
}

