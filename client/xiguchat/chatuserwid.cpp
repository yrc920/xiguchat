#include "chatuserwid.h"
#include "ui_chatuserwid.h"

ChatUserWid::ChatUserWid(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ChatUserWid)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CHAT_USER_ITEM); //设置列表项的类型为聊天用户项
}

ChatUserWid::~ChatUserWid()
{
    delete ui;
}

void ChatUserWid::SetInfo(QString name, QString head, QString msg)
{
    _name = name;
    _head = head;
    _msg = msg;

    QPixmap pixmap(_head);//加载图片
    //设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->user_name_lb->setText(_name); //设置用户名标签的文本为用户名
    ui->user_chat_lb->setText(_msg); //设置用户消息标签的文本为用户消息
}
