#ifndef CHATITEMBASE_H
#define CHATITEMBASE_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include "global.h"
/******************************************************************************
 *
 * @file       chatitembase.h
 * @brief      聊天消息项的基类, 包含了显示用户名、用户头像和消息内容等基本元素
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

class ChatItemBase : public QWidget
{
    Q_OBJECT //使得ChatItemBase可以使用Qt的信号和槽机制

public:
    explicit ChatItemBase(ChatRole role, QWidget *parent = nullptr);
    void setUserName(const QString &name); //设置用户名
    void setUserIcon(const QPixmap &icon); //设置用户头像
    void setWidget(QWidget *w); //设置聊天消息项中的气泡组件

private:
    ChatRole m_role; //消息的发送者
    QLabel *m_pNameLabel; //显示用户名的标签
    QLabel *m_pIconLabel; //显示用户头像的标签
    QWidget *m_pBubble; //显示消息内容的气泡组件
};

#endif // CHATITEMBASE_H
