#include "listitembase.h"
#include <QPainter>
#include <QStyleOption>

ListItemBase::ListItemBase(QWidget *parent) : QWidget(parent)
{

}

void ListItemBase::SetItemType(ListItemType itemType)
{
    _itemType = itemType;
}

ListItemType ListItemBase::GetItemType()
{
    return _itemType;
}

void ListItemBase::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; //创建一个QStyleOption对象
    opt.init(this); //初始化这个对象，使它包含当前widget的状态和属性
    QPainter p(this); //创建一个QPainter对象，并将当前widget作为绘制设备
    //调用当前widget的style来绘制基本元素(PE_Widget)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
