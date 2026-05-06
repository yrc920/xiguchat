#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
/******************************************************************************
 *
 * @file       chatdialog.h
 * @brief      聊天界面
 *
 * @author     西故
 * @date       2026/05/05
 * @history
 *****************************************************************************/

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT //使得ChatDialog可以使用Qt的信号和槽机制

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addChatUserList(); //添加聊天用户列表项(测试用例)

private:
    void ShowSearch(bool bsearch); //显示或隐藏搜索框

    Ui::ChatDialog *ui;
    ChatUIMode _mode; //当前界面模式
    ChatUIMode _state; //当前界面状态
    bool _b_loading; //是否正在加载更多聊天用户

private slots:
    void slot_loading_chat_user(); //加载更多聊天用户的槽函数
};

#endif // CHATDIALOG_H
