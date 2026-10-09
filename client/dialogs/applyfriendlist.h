#ifndef APPLYFRIENDLIST_H
#define APPLYFRIENDLIST_H

#include <QListWidget>
#include <QEvent>
/******************************************************************************
 *
 * @file       applyfriendlist.h
 * @brief      好友申请列表控件, 显示好友申请列表中的所有好友申请条目
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

class ApplyFriendList: public QListWidget
{
    Q_OBJECT //使得ApplyFriendList可以使用Qt的信号和槽机制

public:
    ApplyFriendList(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override; //重写事件过滤器

private slots:

signals:
    void sig_show_search(bool); //通知主界面显示搜索界面的信号
};

#endif // APPLYFRIENDLIST_H
