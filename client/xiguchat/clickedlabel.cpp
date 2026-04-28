#include "clickedlabel.h"
#include <QMouseEvent>

ClickedLabel::ClickedLabel(QWidget* parent):QLabel (parent), _curstate(ClickLbState::Normal) {}

void ClickedLabel::mousePressEvent(QMouseEvent* event)  {
    if (event->button() == Qt::LeftButton) {
        //如果当前状态是正常状态
        if(_curstate == ClickLbState::Normal){
            qDebug()<<"clicked , change to selected hover: "<< _selected_hover;
            _curstate = ClickLbState::Selected; //切换到选中状态
            //设置状态属性为选中悬停状态的样式名称(用于qss根据状态属性选择样式)
            setProperty("state", _selected_hover);
            repolish(this); //刷新样式表，使得状态生效
            update(); //更新界面

        }else{
            qDebug()<<"clicked , change to normal hover: "<< _normal_hover;
            _curstate = ClickLbState::Normal; //切换到正常状态
            //设置状态属性为正常悬停状态的样式名称(用于qss根据状态属性选择样式)
            setProperty("state", _normal_hover);
            repolish(this); //刷新样式表，使得状态生效
            update(); //更新界面
        }
        emit clicked(); //发出clicked信号，通知外部标签被点击了
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);
}

void ClickedLabel::enterEvent(QEvent* event) {
    //如果当前状态是正常状态
    if(_curstate == ClickLbState::Normal){
        qDebug()<<"enter , change to normal hover: "<< _normal_hover;
        //设置状态属性为正常悬停状态的样式名称(用于qss根据状态属性选择样式)
        setProperty("state", _normal_hover);
        repolish(this); //刷新样式表，使得状态生效
        update(); //更新界面

    }else{
        qDebug()<<"enter , change to selected hover: "<< _selected_hover;
        //设置状态属性为选中悬停状态的样式名称(用于qss根据状态属性选择样式)
        setProperty("state",_selected_hover);
        repolish(this); //刷新样式表，使得状态生效
        update(); //更新界面
    }

    QLabel::enterEvent(event); //调用基类的enterEvent以保证正常的事件处理
}

void ClickedLabel::leaveEvent(QEvent* event){
    //如果当前状态是正常状态
    if(_curstate == ClickLbState::Normal){
        qDebug()<<"leave , change to normal : "<< _normal;
        //设置状态属性为正常状态的样式名称(用于qss根据状态属性选择样式)
        setProperty("state", _normal);
        repolish(this); //刷新样式表，使得状态生效
        update(); //更新界面

    }else{
        qDebug()<<"leave , change to normal hover: "<< _selected;
        //设置状态属性为选中状态的样式名称(用于qss根据状态属性选择样式)
        setProperty("state", _selected);
        repolish(this); //刷新样式表，使得状态生效
        update(); //更新界面
    }

    QLabel::leaveEvent(event); //调用基类的leaveEvent以保证正常的事件处理
}

void ClickedLabel::SetState(QString normal, QString hover, QString press,
    QString select, QString select_hover, QString select_press)
{
    //设置状态的样式名称
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    //设置初始状态为正常状态的样式名称(用于qss根据状态属性选择样式)
    setProperty("state", normal);
    repolish(this); //刷新样式表，使得状态生效
    update(); //更新界面
}

ClickLbState ClickedLabel::GetCurState(){
    return _curstate; //返回当前状态
}
