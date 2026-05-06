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
    void paintEvent(QPaintEvent *event);

private:
    ListItemType _itemType; //列表项的类型

public slots:

signals:

};

#endif // LISTITEMBASE_H
