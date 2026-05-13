#ifndef GROUPTIPITEM_H
#define GROUPTIPITEM_H

#include <QWidget>
#include "listitembase.h"
/******************************************************************************
 *
 * @file       grouptipitem.h
 * @brief      分组提示条目, 显示在联系人列表中, 用于区分不同的分组
 *
 * @author     西故
 * @date       2026/05/13
 * @history
 *****************************************************************************/

namespace Ui {
class GroupTipItem;
}

class GroupTipItem : public ListItemBase
{
    Q_OBJECT //使得GroupTipItem可以使用Qt的信号和槽机制

public:
    explicit GroupTipItem(QWidget *parent = nullptr);
    ~GroupTipItem();
    QSize sizeHint() const override; //重写sizeHint函数, 返回自定义的尺寸
    void SetGroupTip(QString str); //设置分组提示文本

private:
    Ui::GroupTipItem *ui;
    QString _tip; //分组提示文本
};

#endif // GROUPTIPITEM_H
