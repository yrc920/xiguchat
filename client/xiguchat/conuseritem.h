#ifndef CONUSERITEM_H
#define CONUSERITEM_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"
/******************************************************************************
 *
 * @file       conuseritem.h
 * @brief      联系人条目, 显示在联系人列表中
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

namespace Ui {
class ConUserItem;
}

class ConUserItem : public ListItemBase
{
    Q_OBJECT //使得ConUserItem可以使用Qt的信号和槽机制

public:
    explicit ConUserItem(QWidget *parent = nullptr);
    ~ConUserItem();
    QSize sizeHint() const override; //重写sizeHint函数, 返回自定义的尺寸
    void SetInfo(std::shared_ptr<AuthInfo> auth_info); //设置用户信息
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp); //设置用户信息
    void SetInfo(int uid, QString name, QString icon); //设置用户信息
    void ShowRedPoint(bool show = false); //显示或隐藏红点提示
    std::shared_ptr<UserInfo> GetInfo(); //获取用户信息结构体

private:
    Ui::ConUserItem *ui;
    std::shared_ptr<UserInfo> _info; //用户信息结构体
};

#endif // CONUSERITEM_H
