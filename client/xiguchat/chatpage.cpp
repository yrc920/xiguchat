#include "chatpage.h"
#include "ui_chatpage.h"
#include <QPainter>
#include <QStyleOption>
#include "chatitembase.h"
#include "textbubble.h"
#include "picturebubble.h"

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    //设置按钮样式
    ui->receive_btn->SetState("normal", "hover","press");
    ui->send_btn->SetState("normal", "hover", "press");

    //设置图标样式
    ui->emo_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
    ui->file_lb->SetState("normal", "hover", "press", "normal", "hover", "press");
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; //创建一个QStyleOption对象
    opt.init(this); //初始化这个对象，使它包含当前widget的状态和属性
    QPainter p(this); //创建一个QPainter对象，并将当前widget作为绘制设备
    //调用当前widget的style来绘制基本元素(PE_Widget)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_btn_clicked()
{
    auto pTextEdit = ui->chatEdit; //获取消息输入区的指针
    ChatRole role = ChatRole::Self; //定义一个ChatRole枚举变量，表示消息的发送者是自己
    //定义发送者的用户名和头像路径
    QString userName = QStringLiteral("西故");
    QString userIcon = ":/res/head_1.jpg";

    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList(); //获取消息输入区中的消息列表，这个列表包含了用户输入的文本、图片和文件等信息
    for(int i = 0; i<msgList.size(); ++i)
    {
        QString type = msgList[i].msgFlag; //获取消息的类型，这个类型可以是"text"、"image"或"file"
        //创建一个新的ChatItemBase对象，表示一条聊天消息项，并将发送者的角色作为参数传入构造函数
        ChatItemBase *pChatItem = new ChatItemBase(role);
        //设置聊天消息项的用户名和头像
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));

        QWidget *pBubble = nullptr; //定义一个指向QWidget的指针，表示聊天消息项中的气泡组件
        //根据消息的类型创建不同的气泡组件, 并将消息内容作为参数传入构造函数
        if(type == "text")
        {
            pBubble = new TextBubble(role, msgList[i].content);
        }
        else if(type == "image")
        {
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
        }
        else if(type == "file")
        {

        }

        //如果成功创建了气泡组件
        if(pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble); //将气泡组件设置到聊天消息项中
            ui->chat_data_list->appendChatItem(pChatItem); //将聊天消息项添加到消息显示区的列表中
        }
    }
}

