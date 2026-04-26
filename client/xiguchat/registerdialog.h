#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"
/******************************************************************************
 *
 * @file       registerdialog.h
 * @brief      注册界面
 *
 * @author     西故
 * @date       2026/04/20
 * @history
 *****************************************************************************/
namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_get_code_clicked(); //获取验证码按钮的槽函数
    //注册模块http请求完成的槽函数
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    void on_sure_btn_clicked(); //注册按钮的槽函数

private:
    Ui::RegisterDialog *ui;
    //请求id和对应的处理函数的映射表,当http请求完成时,根据请求id找到对应的处理函数进行处理
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    void showTip(QString str, bool bo_k); //显示提示信息
    void initHttpHandlers(); //初始化http响应处理函数
};

#endif // REGISTERDIALOG_H
