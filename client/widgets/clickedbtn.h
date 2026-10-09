#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QPushButton>
/******************************************************************************
 *
 * @file       clickedbtn.h
 * @brief      可点击的按钮控件
 *
 * @author     西故
 * @date       2026/05/05
 * @history
 *****************************************************************************/

class ClickedBtn : public QPushButton
{
    Q_OBJECT //使得ClickedBtn可以使用Qt的信号和槽机制

public:
    ClickedBtn(QWidget * parent = nullptr);
    ~ClickedBtn();
    void SetState(QString nomal, QString hover, QString press); //设置状态的样式名称

protected:
    virtual void enterEvent(QEvent *event) override; //鼠标进入
    virtual void leaveEvent(QEvent *event) override; //鼠标离开
    virtual void mousePressEvent(QMouseEvent *event) override; //鼠标按下
    virtual void mouseReleaseEvent(QMouseEvent *event) override; //鼠标释放

private:
    QString _normal; //正常状态的样式名称
    QString _hover; //悬停状态的样式名称
    QString _press; //按下状态的样式名称
};

#endif // CLICKEDBTN_H
