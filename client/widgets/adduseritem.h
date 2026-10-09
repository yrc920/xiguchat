#ifndef ADDUSERITEM_H
#define ADDUSERITEM_H

#include <QWidget>
#include "listitembase.h"
/******************************************************************************
 *
 * @file       adduseritem.h
 * @brief      添加用户列表项
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

namespace Ui {
class AddUserItem;
}

class AddUserItem : public ListItemBase
{
    Q_OBJECT //使得AddUserItem可以使用Qt的信号和槽机制

public:
    explicit AddUserItem(QWidget *parent = nullptr);
    ~AddUserItem();
    QSize sizeHint() const override {
        return QSize(400, 120); // 返回自定义的尺寸
    }

protected:

private:
    Ui::AddUserItem *ui;
};


#endif // ADDUSERITEM_H
