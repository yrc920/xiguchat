#ifndef CONTACTUSERLIST_H
#define CONTACTUSERLIST_H

#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QDebug>
#include <memory>
#include "userdata.h"
class ConUserItem;
/******************************************************************************
 *
 * @file       contactuserlist.h
 * @brief      联系人列表控件, 显示好友列表和新的朋友申请等相关信息
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

class ContactUserList : public QListWidget
{
    Q_OBJECT //使得ContactUserList可以使用Qt的信号和槽机制

public:
    ContactUserList(QWidget* parent = nullptr);
    void ShowRedPoint(bool bshow = true); //显示新的朋友申请的红点提示

protected:
    bool eventFilter(QObject *watched, QEvent *event) override; //重写事件过滤器

private:
    void addContactUserList(); //模拟从数据库或者后端传输过来的数据, 进行列表加载

    bool _load_pending; //是否正在加载更多联系人用户的标志
    ConUserItem* _add_friend_item; //新的朋友申请条目对象
    QListWidgetItem * _groupitem; //联系人分组提示条目对象

public slots:
    void slot_item_clicked(QListWidgetItem *item); //处理联系人列表中条目被点击的槽函数
    // void slot_add_auth_firend(std::shared_ptr<AuthInfo>);
    // void slot_auth_rsp(std::shared_ptr<AuthRsp>);

signals:
    void sig_loading_contact_user(); //通知聊天界面加载更多联系人用户的信号
    void sig_switch_apply_friend_page(); //通知主界面切换到好友申请界面的信号
    //通知主界面切换到好友信息界面的信号, 并传递用户信息
    void sig_switch_friend_info_page(std::shared_ptr<UserInfo> user_info);
};

#endif // CONTACTUSERLIST_H
