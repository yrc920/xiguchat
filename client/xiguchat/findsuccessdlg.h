#ifndef FINDSUCCESSDLG_H
#define FINDSUCCESSDLG_H

#include <QDialog>
#include <memory>
#include "userdata.h"
/******************************************************************************
 *
 * @file       findsuccessdlg.h
 * @brief      搜索成功对话框, 用于在搜索用户成功后显示用户信息和添加好友按钮
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

namespace Ui {
class FindSuccessDlg;
}

class FindSuccessDlg : public QDialog
{
    Q_OBJECT //使得FindSuccessDlg可以使用Qt的信号和槽机制

public:
    explicit FindSuccessDlg(QWidget *parent = nullptr);
    ~FindSuccessDlg();
    void SetSearchInfo(std::shared_ptr<SearchInfo> si); //设置搜索信息, 用于在对话框中显示用户信息

private slots:
    void on_add_friend_btn_clicked(); //添加好友按钮点击的槽函数

private:
    Ui::FindSuccessDlg *ui;
    QWidget * _parent; //父窗口指针, 用于在添加好友按钮点击时弹出加好友界面
    std::shared_ptr<SearchInfo> _si; //搜索信息的智能指针
};

#endif // FINDSUCCESSDLG_H
