#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent) : QPushButton(parent), _counter(10)
{
    _timer = new QTimer(this); //创建定时器对象，并将其父对象设置为当前按钮，以便自动管理内存

    //连接定时器的timeout信号到一个lambda表达式，用于更新按钮的文本和状态
    connect(_timer, &QTimer::timeout, [this](){
        _counter--; //每次定时器触发时，倒计时数字减1
        //当倒计时结束时，停止定时器，重置按钮文本和状态
        if(_counter <= 0){
            _timer->stop();
            _counter = 10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter)); //更新按钮文本为当前倒计时数字
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop(); //确保定时器停止，避免在对象销毁后继续触发
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    //如果是鼠标左键释放事件
    if (e->button() == Qt::LeftButton) {
        qDebug() << "MyButton was released!";
        this->setEnabled(false); //禁用按钮，防止重复点击
        this->setText(QString::number(_counter)); //显示初始倒计时数字
        _timer->start(1000); //每隔1秒触发一次，更新倒计时
        emit clicked(); //发出clicked信号(qt低版本需要手动发出)
    }

    //调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}
