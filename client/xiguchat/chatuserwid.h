#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include <QWidget>
#include "listitembase.h"
/******************************************************************************
 *
 * @file       chatuserwid.h
 * @brief      聊天用户项, 显示用户头像、用户名和最新消息
 *
 * @author     西故
 * @date       2026/05/06
 * @history
 *****************************************************************************/

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT //使得ChatUserWid可以使用Qt的信号和槽机制

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(350, 110); //返回自定义的尺寸
    }

    void SetInfo(QString name, QString head, QString msg); //设置用户信息

private:
    Ui::ChatUserWid *ui;
    QString _name; //用户名
    QString _head; //用户头像路径
    QString _msg; //用户消息
};

#endif // CHATUSERWID_H
