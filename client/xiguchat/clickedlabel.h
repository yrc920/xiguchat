#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include "global.h"
/*****************************************************
 *
 * @file       clickedlabel.h
 * @brief      可点击的标签控件
 *
 * @author     西故
 * @date       2026/04/27
 * @history
 *****************************************************/

class ClickedLabel : public QLabel
{
    Q_OBJECT //使得ClickedLabel可以使用Qt的信号和槽机制

public:
    ClickedLabel(QWidget* parent);
    virtual void mousePressEvent(QMouseEvent *ev) override; //处理鼠标点击事件
    virtual void enterEvent(QEvent* event) override; //处理鼠标悬停进入事件
    virtual void leaveEvent(QEvent* event) override; //处理鼠标悬停离开事件
    //设置状态的样式名称
    void SetState(QString normal = "", QString hover = "", QString press = "",
        QString select = "", QString select_hover = "", QString select_press = "");

    ClickLbState GetCurState(); //获取当前状态

private:
    QString _normal; //正常状态的样式名称
    QString _normal_hover; //正常悬停状态的样式名称
    QString _normal_press; //正常按下状态的样式名称(和选中悬停状态相同)

    QString _selected; //选中状态的样式名称
    QString _selected_hover; //选中悬停状态的样式名称
    QString _selected_press; //选中按下状态的样式名称(和正常悬停状态相同))

    ClickLbState _curstate; //当前状态

signals:
    void clicked(void);

};

#endif // CLICKEDLABEL_H
