#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"
/******************************************************************************
 *
 * @file       chatuserwid.h
 * @brief      聊天用户项, 显示用户头像、用户名和最新消息
 *
 * @author     西故
 * @date       2026/05/06
 * @history
 *****************************************************************************/

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT //使得ChatUserWid可以使用Qt的信号和槽机制

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(350, 110); //返回自定义的尺寸
    }

    void SetInfo(std::shared_ptr<UserInfo> user_info);
    void SetInfo(std::shared_ptr<FriendInfo> friend_info);
    std::shared_ptr<UserInfo> GetUserInfo();

private:
    Ui::ChatUserWid *ui;
    std::shared_ptr<UserInfo> _user_info;
};

#endif // CHATUSERWID_H
