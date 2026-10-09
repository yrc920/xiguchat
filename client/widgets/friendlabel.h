#ifndef FRIENDLABEL_H
#define FRIENDLABEL_H

#include <QFrame>
#include <QString>
/******************************************************************************
 *
 * @file       friendlabel.h
 * @brief      好友标签控件, 用于在申请好友界面显示已经添加的好友标签, 包括标签文本和关闭按钮
 *
 * @author     西故
 * @date       2026/05/11
 * @history
 *****************************************************************************/

namespace Ui {
class FriendLabel;
}

class FriendLabel : public QFrame
{
    Q_OBJECT //使得FriendLabel可以使用Qt的信号和槽机制

public:
    explicit FriendLabel(QWidget *parent = nullptr);
    ~FriendLabel();
    void SetText(QString text); //设置标签文本, 并根据文本内容调整标签的宽度和高度以适应文本显示
    int Width(); //获取标签的宽度
    int Height(); //获取标签的高度
    QString Text(); //获取标签的文本内容

private:
    Ui::FriendLabel *ui;
    QString _text; //标签文本内容
    int _width; //标签的宽度
    int _height; //标签的高度

public slots:
    void slot_close(); //关闭标签的槽函数

signals:
    void sig_close(QString); //关闭标签的信号
};


#endif // FRIENDLABEL_H
