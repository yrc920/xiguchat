#ifndef CLICKEDONCELABEL_H
#define CLICKEDONCELABEL_H

#include <QLabel>
#include <QMouseEvent>
/******************************************************************************
 *
 * @file       clickedoncelabel.h
 * @brief      单击一次就触发clicked信号的标签
 *
 * @author     西故
 * @date       2026/05/11
 * @history
 *****************************************************************************/

class ClickedOnceLabel:public QLabel
{
    Q_OBJECT //使得ClickedOnceLabel可以使用Qt的信号和槽机制

public:
    ClickedOnceLabel(QWidget *parent=nullptr);
    virtual void mouseReleaseEvent(QMouseEvent *ev) override; //处理鼠标释放事件

signals:
    void clicked(QString); //标签被点击的信号, 传递标签的文本
};

#endif // CLICKEDONCELABEL_H
