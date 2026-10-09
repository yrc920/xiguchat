#ifndef STATEWIDGET_H
#define STATEWIDGET_H

#include <QWidget>
#include "global.h"
#include <QLabel>
/******************************************************************************
 *
 * @file       statewidget.h
 * @brief      状态控件, 用于实现聊天界面侧边栏的聊天和联系人的状态切换
 *
 * @author     西故
 * @date       2026/05/09
 * @history
 *****************************************************************************/

class StateWidget : public QWidget
{
    Q_OBJECT //使得StateWidget可以使用Qt的信号和槽机制

public:
    explicit StateWidget(QWidget *parent = nullptr);

    void SetState(QString normal="", QString hover="", QString press="",
        QString select="", QString select_hover="", QString select_press=""); //设置状态样式

    ClickLbState GetCurState(); //获取当前状态
    void ClearState(); //清除状态, 恢复到初始状态

    void SetSelected(bool bselected); //设置是否选中状态
    void AddRedPoint(); //添加红点示意图
    void ShowRedPoint(bool show=true); //显示或隐藏红点示意图

protected:
    void paintEvent(QPaintEvent* event); //重写paintEvent事件, 用于自定义绘制控件的外观

    virtual void mousePressEvent(QMouseEvent *ev) override; //处理鼠标点击事件
    virtual void mouseReleaseEvent(QMouseEvent *ev) override; //处理鼠标释放事件
    virtual void enterEvent(QEvent* event) override; //处理鼠标悬停事件
    virtual void leaveEvent(QEvent* event) override; //处理鼠标悬停离开事件

private:

    QString _normal; //正常状态的样式
    QString _normal_hover; //悬停状态的样式
    QString _normal_press; //按下状态的样式

    QString _selected; //选中状态的样式
    QString _selected_hover; //选中悬停状态的样式
    QString _selected_press; //选中按下状态的样式

    ClickLbState _curstate; //当前状态
    QLabel * _red_point; //红点示意图

signals:
    void clicked(void); //点击信号, 当鼠标点击控件时发出, 用于通知外部进行状态切换或其他操作

signals:

public slots:
};

#endif // STATEWIDGET_H
