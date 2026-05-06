#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H

#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
/******************************************************************************
 *
 * @file       chatuserlist.h
 * @brief      聊天用户列表, 显示所有的聊天用户, 支持滚轮加载更多用户
 *
 * @author     西故
 * @date       2026/05/06
 * @history
 *****************************************************************************/

class ChatUserList: public QListWidget
{
    Q_OBJECT //使得ChatUserList可以使用Qt的信号和槽机制

public:
    ChatUserList(QWidget *parent = nullptr);

protected:
    //重写事件过滤器函数, 用于处理鼠标悬浮和滚轮事件
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sig_loading_chat_user(); //当滚轮滚动到底部时发送信号通知聊天界面加载更多聊天用户
};

#endif // CHATUSERLIST_H
