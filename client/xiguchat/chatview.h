#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>
/******************************************************************************
 *
 * @file       chatview.h
 * @brief      聊天显示区, 用于显示聊天内容, 支持滚动和动态添加聊天项
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

class ChatView : public QWidget
{
    Q_OBJECT //使得ChatView可以使用Qt的信号和槽机制

public:
    ChatView(QWidget *parent = Q_NULLPTR);
    void appendChatItem(QWidget *item); //在布局中添加一个聊天项, 使其显示在底部
    void prependChatItem(QWidget *item); //在布局中添加一个聊天项, 使其显示在顶部
    void insertChatItem(QWidget *before, QWidget *item); //在布局中插入一个聊天项, 使其显示在指定项之前

protected:
    bool eventFilter(QObject *o, QEvent *e) override; //重写事件过滤器函数, 用于处理滚动区域的事件
    //重写paintEvent函数, 使得这个widget能够正确地绘制自己的背景和边框等基本元素
    void paintEvent(QPaintEvent *event) override;

private:
    void initStyleSheet();

    //QWidget *m_pCenterWidget;
    QVBoxLayout *m_pVl; //滚动区域内的垂直布局, 用于排列聊天项
    QScrollArea *m_pScrollArea; //滚动区域, 用于显示聊天项并支持滚动
    bool isAppended; //标志位, 用于指示是否已经添加了聊天项

private slots:
    void onVScrollBarMoved(int min, int max); //当滚动条移动时的槽函数, 用于调整滚动位置以保持在底部
};

#endif // CHATVIEW_H
