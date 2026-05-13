#include "applyfrienditem.h"
#include "ui_applyfrienditem.h"

ApplyFriendItem::ApplyFriendItem(QWidget *parent) :
    ListItemBase(parent), _added(false),
    ui(new Ui::ApplyFriendItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::APPLY_FRIEND_ITEM); //设置条目类型为好友申请条目
    ui->addBtn->SetState("normal", "hover", "press"); //设置按钮的状态图片
    ui->addBtn->hide(); //默认隐藏添加按钮

    //连接添加按钮的点击信号到发送好友认证信号的槽函数
    connect(ui->addBtn, &ClickedBtn::clicked,  [this](){
        emit this->sig_auth_friend(_apply_info); //发送好友认证信号, 并传递好友申请信息
    });
}

ApplyFriendItem::~ApplyFriendItem()
{
    delete ui;
}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info; //将好友申请信息存储在成员变量中

    QPixmap pixmap(_apply_info->_icon); //加载图片
    //设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->user_name_lb->setText(_apply_info->_name); //设置用户名标签的文本为申请人的名字
    ui->user_chat_lb->setText(_apply_info->_desc); //设置用户备注标签的文本为申请人的备注信息

}

void ApplyFriendItem::ShowAddBtn(bool bshow)
{
    //如果bshow为true
    if (bshow)
    {
        ui->addBtn->show(); //显示添加按钮
        ui->already_add_lb->hide(); //隐藏已经添加的标签
        _added = false; //将_added标志设置为false, 表示还没有添加好友
    }
    else {
        ui->addBtn->hide(); //隐藏添加按钮
        ui->already_add_lb->show(); //显示已经添加的标签
        _added = true; //将_added标志设置为true, 表示已经添加好友
    }
}

int ApplyFriendItem::GetUid()
{
    return _apply_info->_uid; //返回申请人的用户ID
}
