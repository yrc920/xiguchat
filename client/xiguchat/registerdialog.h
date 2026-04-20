#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
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
    void on_get_code_clicked();

private:
    Ui::RegisterDialog *ui;
    void showTip(QString str, bool bo_k); //显示提示信息
};

#endif // REGISTERDIALOG_H
