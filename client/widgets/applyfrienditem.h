#ifndef APPLYFRIENDITEM_H
#define APPLYFRIENDITEM_H

#include <QWidget>
#include <listitembase.h>
#include "userdata.h"
#include <memory>
/******************************************************************************
 *
 * @file       applyfrienditem.h
 * @brief      好友申请条目, 显示在好友申请列表中, 用于显示每个好友申请的信息和操作按钮
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

namespace Ui {
class ApplyFriendItem;
}

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT //使得ApplyFriendItem可以使用Qt的信号和槽机制

public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info); //设置好友申请信息
    void ShowAddBtn(bool bshow); //显示或隐藏添加按钮
    QSize sizeHint() const override {
        return QSize(400, 130); //返回自定义的尺寸
    }
    int GetUid(); //获取申请人的用户ID

private:
    Ui::ApplyFriendItem *ui;
    std::shared_ptr<ApplyInfo> _apply_info; //好友申请信息结构体
    bool _added; //是否已经添加好友的标志

signals:
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info); //发送好友认证信号, 并传递好友申请信息
};

#endif // APPLYFRIENDITEM_H
