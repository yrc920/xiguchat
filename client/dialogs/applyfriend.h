#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include <QDialog>
#include "clickedlabel.h"
#include "friendlabel.h"
#include "userdata.h"
/******************************************************************************
 *
 * @file       applyfriend.h
 * @brief      申请好友界面, 包括输入框、标签展示区、提示标签等功能
 *
 * @author     西故
 * @date       2026/05/11
 * @history
 *****************************************************************************/

namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT //使得ApplyFriend可以使用Qt的信号和槽机制

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();
    void InitTipLbs(); //初始化提示标签
    //将标签添加到标签展示区, 并更新下一个标签的位置
    void AddTipLbs(ClickedLabel*, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event); //事件过滤器
    void SetSearchInfo(std::shared_ptr<SearchInfo> si); //设置搜索到的用户信息

private:
    void resetLabels(); //根据_friend_labels中的标签重新布局标签展示区的标签
    void addLabel(QString name); //添加标签到标签展示区

    Ui::ApplyFriend *ui;
    QPoint _label_point; //标签展示区当前坐标
    QMap<QString, FriendLabel*> _friend_labels; //输入框显示的标签map
    std::vector<QString> _friend_label_keys; //输入框显示的标签的key列表

    QMap<QString, ClickedLabel*> _add_labels; //已经创建好的标签
    std::vector<QString> _add_label_keys; //已经创建好的标签的key列表
    std::vector<QString> _tip_data; //已经添加的标签数据
    QPoint _tip_cur_point; //提示标签当前坐标
    std::shared_ptr<SearchInfo> _si; //搜索到的用户信息

public slots:
    void ShowMoreLabel(); //显示更多label标签
    void SlotLabelEnter(); //输入label按下回车触发将标签加入展示栏
    void SlotRemoveFriendLabel(QString); //点击关闭, 移除展示栏好友便签
    void SlotChangeFriendLabelByTip(QString, ClickLbState); //通过点击tip实现增加和减少好友便签
    void SlotLabelTextChange(const QString& text); //输入框文本变化显示不同提示
    void SlotAddFirendLabelByClickTip(QString text); //输入标签显示提示框，点击提示框内容后添加好友便签
    void SlotApplySure(); //处理确认回调
    void SlotApplyCancel(); //处理取消回调
};

#endif // APPLYFRIEND_H
