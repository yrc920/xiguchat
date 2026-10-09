#ifndef TEXTBUBBLE_H
#define TEXTBUBBLE_H

#include <QTextEdit>
#include "bubbleframe.h"
#include <QHBoxLayout>
/******************************************************************************
 *
 * @file       textbubble.h
 * @brief      聊天消息项中的文本气泡组件
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

class TextBubble : public BubbleFrame
{
    Q_OBJECT //使得TextBubble可以使用Qt的信号和槽机制

public:
    TextBubble(ChatRole role, const QString &text, QWidget *parent = nullptr);

protected:
    //重写eventFilter函数, 用于监听文本编辑区的内容变化事件, 以调整文本气泡的高度
    bool eventFilter(QObject *o, QEvent *e);

private:
    void adjustTextHeight(); //调整文本气泡的高度, 使其能够适应文本内容的高度
    void setPlainText(const QString &text); //设置文本编辑区的纯文本内容, 并调整文本气泡的高度
    void initStyleSheet(); //初始化文本气泡的样式表

    QTextEdit *m_pTextEdit; //文本编辑区组件, 用于显示文本消息内容
};


#endif // TEXTBUBBLE_H
