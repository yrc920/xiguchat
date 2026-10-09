#ifndef BUBBLEFRAME_H
#define BUBBLEFRAME_H

#include <QFrame>
#include "global.h"
#include <QHBoxLayout>
/******************************************************************************
 *
 * @file       bubbleframe.h
 * @brief      聊天消息项中的气泡组件的基类, 包含了绘制气泡背景和设置气泡内容等基本功能
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

class BubbleFrame : public QFrame
{
    Q_OBJECT //使得BubbleFrame可以使用Qt的信号和槽机制

public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void setMargin(int margin); //设置气泡组件的内边距
    //inline int margin(){return margin;}
    void setWidget(QWidget *w); //设置气泡组件中的内容组件

protected:
    void paintEvent(QPaintEvent *e); //重写paintEvent函数, 绘制气泡组件的背景和小三角等元素

private:
    QHBoxLayout *m_pHLayout; //气泡组件的水平布局管理器, 用于管理气泡组件中的内容组件
    ChatRole m_role; //消息的发送者
    int m_margin; //气泡组件的内边距
};

#endif // BUBBLEFRAME_H
