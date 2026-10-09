#include "applyfriendlist.h"
#include <QWheelEvent>
#include <QScrollBar>
#include "listitembase.h"

ApplyFriendList::ApplyFriendList(QWidget *parent)
{
    Q_UNUSED(parent);
    //默认隐藏滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->viewport()->installEventFilter(this); //安装事件过滤器
}

bool ApplyFriendList::eventFilter(QObject *watched, QEvent *event)
{
    //检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport())
    {
        if (event->type() == QEvent::Enter) {
            //鼠标悬浮, 显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            //鼠标离开, 隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    //检查事件是否是鼠标点击事件
    if (watched == this->viewport())
    {
        //如果是鼠标点击事件
        if (event->type() == QEvent::MouseButtonPress) {
            emit sig_show_search(false); //发送信号通知主界面显示搜索界面
        }
    }

    //检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event); //将事件转换为QWheelEvent类型
        int numDegrees = wheelEvent->angleDelta().y() / 8; // 获取滚轮滚动的角度
        int numSteps = numDegrees / 15; // 计算滚动步数

        //设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        return true; // 停止事件传递
    }
    return QListWidget::eventFilter(watched, event); //对于其他事件, 调用基类的事件过滤器进行默认处理
}
