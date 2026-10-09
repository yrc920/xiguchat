#ifndef APPLYFRIENDPAGE_H
#define APPLYFRIENDPAGE_H

#include <QWidget>
#include "userdata.h"
#include <memory>
#include <QJsonArray>
#include <unordered_map>
#include "applyfrienditem.h"
/******************************************************************************
 *
 * @file       applyfriendpage.h
 * @brief      好友申请界面, 显示好友申请列表和处理好友申请相关的操作
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

namespace Ui {
class ApplyFriendPage;
}

class ApplyFriendPage : public QWidget
{
    Q_OBJECT //使得ApplyFriendPage可以使用Qt的信号和槽机制

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();
    void AddNewApply(std::shared_ptr<AddFriendApply> apply); //添加新的好友申请到好友申请列表中

protected:
    void paintEvent(QPaintEvent *event); //重写paintEvent函数

private:
    void loadApplyList(); //加载好友申请列表, 模拟从数据库或者后端传输过来的数据, 进行列表加载

    Ui::ApplyFriendPage *ui;
    std::unordered_map<int, ApplyFriendItem*> _unauth_items; //未认证的好友申请条目哈希表

public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp>); //处理好友认证响应的槽函数

signals:
    void sig_show_search(bool); //通知主界面显示搜索界面的信号
};


#endif // APPLYFRIENDPAGE_H
