#include "statewidget.h"
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>

StateWidget::StateWidget(QWidget *parent) : QWidget(parent), _curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor); //设置鼠标指针为手形, 表示这个控件是可点击的

    AddRedPoint(); //添加红点
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; //创建一个QStyleOption对象
    opt.init(this); //初始化这个对象，使它包含当前widget的状态和属性
    QPainter p(this); //创建一个QPainter对象，并将当前widget作为绘制设备
    //调用当前widget的style来绘制基本元素(PE_Widget)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    return;
}

void StateWidget::mousePressEvent(QMouseEvent* event)  {
    //如果点击的是左键
    if (event->button() == Qt::LeftButton) {
        //如果当前状态已经是选中状态, 则不需要切换状态
        if(_curstate == ClickLbState::Selected){
            qDebug() << "PressEvent , already to selected press: " << _selected_press;
            //调用基类的mousePressEvent以保证正常的事件处理
            QWidget::mousePressEvent(event);
            return;
        }

        //如果当前状态是正常状态, 则切换到选中按下状态
        if(_curstate == ClickLbState::Normal){
            qDebug()<<"PressEvent , change to selected press: "<< _selected_press;
            _curstate = ClickLbState::Selected; //切换到选中状态
            setProperty("state", _selected_press); //设置状态属性为选中按下状态的样式
            repolish(this); //重新应用样式表以更新控件的外观
            update(); //更新控件以触发重绘
        }
        return;
    }
    //调用基类的mousePressEvent以保证正常的事件处理
    QWidget::mousePressEvent(event);
}

void StateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    //如果释放的是左键
    if (event->button() == Qt::LeftButton) {
        //如果当前状态是正常状态, 则切换到正常悬停状态
        if(_curstate == ClickLbState::Normal){
            //qDebug()<<"ReleaseEvent , change to normal hover: "<< _normal_hover;
            setProperty("state", _normal_hover); //设置状态属性为正常悬停状态的样式
            repolish(this); //重新应用样式表以更新控件的外观
            update(); //更新控件以触发重绘

        }else{
            //qDebug()<<"ReleaseEvent , change to select hover: "<< _selected_hover;
            setProperty("state", _selected_hover); //设置状态属性为选中悬停状态的样式
            repolish(this); //重新应用样式表以更新控件的外观
            update(); //更新控件以触发重绘
        }
        emit clicked(); //发出点击信号, 通知外部进行状态切换或其他操作
        return; //不继续处理其他事件, 直接返回
    }
    //调用基类的mousePressEvent以保证正常的事件处理
    QWidget::mousePressEvent(event);
}

void StateWidget::enterEvent(QEvent* event) {
    //如果鼠标进入控件区域, 则根据当前状态切换到对应的悬停状态
    if(_curstate == ClickLbState::Normal){
        //qDebug()<<"enter , change to normal hover: "<< _normal_hover;
        setProperty("state", _normal_hover); //设置状态属性为正常悬停状态的样式
        repolish(this); //重新应用样式表以更新控件的外观
        update(); //更新控件以触发重绘
    }else{
        //qDebug()<<"enter , change to selected hover: "<< _selected_hover;
        setProperty("state", _selected_hover); //设置状态属性为选中悬停状态的样式
        repolish(this); //重新应用样式表以更新控件的外观
        update(); //更新控件以触发重绘
    }

    QWidget::enterEvent(event);
}

void StateWidget::leaveEvent(QEvent* event){
    //如果鼠标离开控件区域, 则根据当前状态切换到对应的正常状态
    if(_curstate == ClickLbState::Normal){
        // qDebug()<<"leave , change to normal : "<< _normal;
        setProperty("state", _normal); //设置状态属性为正常状态的样式
        repolish(this); //重新应用样式表以更新控件的外观
        update(); //更新控件以触发重绘

    }else{
        // qDebug()<<"leave , change to select normal : "<< _selected;
        setProperty("state", _selected); //设置状态属性为选中状态的样式
        repolish(this); //重新应用样式表以更新控件的外观
        update(); //更新控件以触发重绘
    }
    QWidget::leaveEvent(event);
}

void StateWidget::SetState(QString normal, QString hover, QString press,
    QString select, QString select_hover, QString select_press)
{
    //设置状态样式, 包括正常、悬停、按下和选中状态的样式
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    setProperty("state", normal); //设置状态属性为正常状态的样式
    repolish(this); //重新应用样式表以更新控件的外观
}

ClickLbState StateWidget::GetCurState(){
    return _curstate;
}

void StateWidget::ClearState()
{
    _curstate = ClickLbState::Normal; //清除状态, 恢复到初始状态
    setProperty("state", _normal); //设置状态属性为正常状态的样式
    repolish(this); //重新应用样式表以更新控件的外观
    update(); //更新控件以触发重绘
}

void StateWidget::SetSelected(bool bselected)
{
    //如果传入的参数表示需要设置为选中状态, 则切换到选中状态的样式
    if(bselected){
        _curstate = ClickLbState::Selected; //设置为选中状态
        setProperty("state", _selected); //设置状态属性为选中状态的样式
        repolish(this); //重新应用样式表以更新控件的外观
        update(); //更新控件以触发重绘
        return;
    }
    //否则切换到正常状态的样式
    _curstate = ClickLbState::Normal; //设置为正常状态
    setProperty("state", _normal); //设置状态属性为正常状态的样式
    repolish(this); //重新应用样式表以更新控件的外观
    update(); //更新控件以触发重绘
    return;

}

void StateWidget::AddRedPoint()
{
    //添加红点示意图
    _red_point = new QLabel();
    _red_point->setObjectName("red_point");
    QVBoxLayout* layout2 = new QVBoxLayout; //创建一个垂直布局管理器
    _red_point->setAlignment(Qt::AlignCenter); //设置红点示意图的对齐方式为居中
    layout2->addWidget(_red_point); //将红点示意图添加到布局管理器中
    layout2->setMargin(0); //设置布局管理器的边距为0
    this->setLayout(layout2); //将布局管理器设置为当前widget的布局
    _red_point->setVisible(false); //初始状态下隐藏红点示意图
}

void StateWidget::ShowRedPoint(bool show)
{
    _red_point->setVisible(true); //显示红点示意图
}
