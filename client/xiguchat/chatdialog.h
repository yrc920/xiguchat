#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
#include "statewidget.h"
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
    void AddLBGroup(StateWidget* lb); //将状态控件添加到状态控件列表中, 以便进行状态管理
    void ClearLabelState(StateWidget* lb); //清除状态控件的状态, 将除了传入的控件以外的其他控件恢复到初始状态

    Ui::ChatDialog *ui;
    ChatUIMode _mode; //当前界面模式
    ChatUIMode _state; //当前界面状态
    bool _b_loading; //是否正在加载更多聊天用户
    QList<StateWidget*> _lb_list; //状态控件列表

private slots:
    void slot_loading_chat_user(); //加载更多聊天用户的槽函数
    void slot_side_chat(); //侧边栏聊天按钮点击的槽函数
    void slot_side_contact(); //侧边栏联系人按钮点击的槽函数
    void slot_text_changed(const QString &str); //搜索框文本改变的槽函数
};

#endif // CHATDIALOG_H
