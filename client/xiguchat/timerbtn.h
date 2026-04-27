#ifndef TIMERBTN_H
#define TIMERBTN_H
#include <QPushButton>
#include <QTimer>
/******************************************************************************
 *
 * @file       timerbtn.h
 * @brief      XXXX Function
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

    // 重写mouseReleaseEvent
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QTimer  *_timer;
    int _counter;
};

#endif // TIMERBTN_H
