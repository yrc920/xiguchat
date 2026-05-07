#ifndef PICTUREBUBBLE_H
#define PICTUREBUBBLE_H

#include "bubbleframe.h"
#include <QHBoxLayout>
#include <QPixmap>
/******************************************************************************
 *
 * @file       picturebubble.h
 * @brief      聊天消息项中的图片气泡组件, 用于显示图片消息内容
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

class PictureBubble : public BubbleFrame
{
    Q_OBJECT //使得PictureBubble可以使用Qt的信号和槽机制

public:
    PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent = nullptr);
};

#endif // PICTUREBUBBLE_H
