#include "clickedbtn.h"
#include "global.h"

ClickedBtn::ClickedBtn(QWidget *parent) : QPushButton (parent)
{
    setCursor(Qt::PointingHandCursor); //设置光标为小手
    setFocusPolicy(Qt::NoFocus); //设置按钮不接受焦点，避免抢占键盘输入
}

ClickedBtn::~ClickedBtn() {}

void ClickedBtn::SetState(QString normal, QString hover, QString press)
{
    //设置状态的样式名称
    _hover = hover;
    _normal = normal;
    _press = press;

    //初始状态为正常状态，设置状态属性为正常状态的样式名称(用于qss根据状态属性选择样式)
    setProperty("state", normal);
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
}

void ClickedBtn::enterEvent(QEvent *event)
{
    setProperty("state", _hover); //设置状态属性为悬停状态的样式名称(用于qss根据状态属性选择样式)
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
    QPushButton::enterEvent(event); //调用基类的enterEvent以保证正常的事件处理
}

void ClickedBtn::leaveEvent(QEvent *event)
{
    setProperty("state", _normal); //设置状态属性为正常状态的样式名称(用于qss根据状态属性选择样式)
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
    QPushButton::leaveEvent(event); //调用基类的leaveEvent以保证正常的事件处理
}

void ClickedBtn::mousePressEvent(QMouseEvent *event)
{
    setProperty("state", _press); //设置状态属性为按下状态的样式名称(用于qss根据状态属性选择样式)
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
    QPushButton::mousePressEvent(event); //调用基类的mousePressEvent以保证正常的事件处理
}

void ClickedBtn::mouseReleaseEvent(QMouseEvent *event)
{
    setProperty("state", _hover); //设置状态属性为悬停状态的样式名称(用于qss根据状态属性选择样式)
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
    QPushButton::mouseReleaseEvent(event); //调用基类的mouseReleaseEvent以保证正常的事件处理
}
