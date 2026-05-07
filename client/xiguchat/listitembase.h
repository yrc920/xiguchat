#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H

#include <QWidget>
#include "global.h"
/******************************************************************************
 *
 * @file       listitembase.h
 * @brief      列表项基类
 *
 * @author     西故
 * @date       2026/05/06
 * @history
 *****************************************************************************/

class ListItemBase : public QWidget
{
    Q_OBJECT //使得ListItemBase可以使用Qt的信号和槽机制

public:
    explicit ListItemBase(QWidget *parent = nullptr);
    void SetItemType(ListItemType itemType); //设置列表项的类型

    ListItemType GetItemType(); //获取列表项的类型

protected:
    //重写paintEvent函数, 使得这个widget能够正确地绘制自己的背景和边框等基本元素
    virtual void paintEvent(QPaintEvent *event) override;

private:
    ListItemType _itemType; //列表项的类型

public slots:

signals:

};

#endif // LISTITEMBASE_H
