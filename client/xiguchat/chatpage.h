#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
/******************************************************************************
 *
 * @file       chatpage.h
 * @brief      聊天界面, 包含消息显示区、消息输入区和发送按钮等元素
 *
 * @author     西故
 * @date       2026/05/07
 * @history
 *****************************************************************************/

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT //使得ChatPage可以使用Qt的信号和槽机制

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();

protected:
    //重写paintEvent函数, 使得这个widget能够正确地绘制自己的背景和边框等基本元素
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void on_send_btn_clicked(); //点击发送按钮时触发的槽函数

private:
    Ui::ChatPage *ui;
};

#endif // CHATPAGE_H
