#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H

#include <QLineEdit>
#include <QDebug>
/******************************************************************************
 *
 * @file       customizeedit.h
 * @brief      搜索框
 *
 * @author     西故
 * @date       2026/05/06
 * @history
 *****************************************************************************/

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT //使得CustomizeEdit可以使用Qt的信号和槽机制

public:
    CustomizeEdit(QWidget *parent = nullptr);
    void SetMaxLength(int maxLen); //设置最大长度

protected:
    void focusOutEvent(QFocusEvent *event) override; //重写失去焦点事件

private:
    void limitTextLength(QString text); //限制文本长度

    int _max_len; //最大长度，单位为字节

signals:
    void sig_foucus_out(); //失去焦点的信号
};

#endif // CUSTOMIZEEDIT_H
