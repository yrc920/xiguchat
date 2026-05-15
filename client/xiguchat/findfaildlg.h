#ifndef FINDFAILDLG_H
#define FINDFAILDLG_H

#include <QDialog>
/******************************************************************************
 *
 * @file       findfaildlg.h
 * @brief      搜索失败对话框, 用于在搜索用户失败后显示错误信息
 *
 * @author     西故
 * @date       2026/05/15
 * @history
 *****************************************************************************/

namespace Ui {
class FindFailDlg;
}

class FindFailDlg : public QDialog
{
    Q_OBJECT //使得FindFailDlg可以使用Qt的信号和槽机制

public:
    explicit FindFailDlg(QWidget *parent = nullptr);
    ~FindFailDlg();

private:
    Ui::FindFailDlg *ui;

private slots:
    void on_fail_sure_btn_clicked(); //搜索失败对话框的确定按钮点击的槽函数, 用于关闭对话框
};

#endif // FINDFAILDLG_H
