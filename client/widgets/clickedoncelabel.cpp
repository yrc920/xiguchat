#include "clickedoncelabel.h"

ClickedOnceLabel::ClickedOnceLabel(QWidget *parent):QLabel(parent)
{
    setCursor(Qt::PointingHandCursor); //设置鼠标悬停时显示手型光标，提示用户该标签是可点击的
}

void ClickedOnceLabel::mouseReleaseEvent(QMouseEvent *event)
{
    //如果鼠标释放事件是由左键触发的
    if (event->button() == Qt::LeftButton) {
        emit clicked(this->text()); //发出clicked信号, 传递标签的文本内容
        return;
    }
    //调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);
}
