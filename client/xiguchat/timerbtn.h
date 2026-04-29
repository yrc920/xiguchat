#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QTimer>
/******************************************************************************
 *
 * @file       timerbtn.h
 * @brief      定时按钮
 *
 * @author     西故
 * @date       2026/04/27
 * @history
 *****************************************************************************/

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr);
    ~ TimerBtn();

    //重写mouseReleaseEvent
    virtual void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QTimer  *_timer; //定时器对象，用于控制倒计时
    int _counter; //倒计时的初始值，表示倒计时的秒数
};

#endif // TIMERBTN_H
