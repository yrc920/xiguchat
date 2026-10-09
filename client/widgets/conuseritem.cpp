#include "conuseritem.h"
#include "ui_conuseritem.h"

ConUserItem::ConUserItem(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ConUserItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CONTACT_USER_ITEM); //设置条目类型为联系人条目
    ui->red_point->raise(); //将红点提示置于最前面, 确保它不会被其他控件遮挡
    ShowRedPoint(false); //默认隐藏红点提示
}

ConUserItem::~ConUserItem()
{
    delete ui;
}

QSize ConUserItem::sizeHint() const
{
    return QSize(350, 110); // 返回自定义的尺寸
}

void ConUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info)
{
    _info = std::make_shared<UserInfo>(auth_info); //将AuthInfo转换为UserInfo并存储在_info成员变量中

    QPixmap pixmap(_info->_icon); //加载图片
    //设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->user_name_lb->setText(_info->_name); //设置用户名标签的文本为用户的名字
}

void ConUserItem::SetInfo(int uid, QString name, QString icon)
{
    //创建一个新的UserInfo对象, 并将其存储在_info成员变量中
    _info = std::make_shared<UserInfo>(uid, name, name, icon, 0);

    QPixmap pixmap(_info->_icon); //加载图片
    //设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->user_name_lb->setText(_info->_name); //设置用户名标签的文本为用户的名字
}

void ConUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp)
{
    _info = std::make_shared<UserInfo>(auth_rsp); //将AuthRsp转换为UserInfo并存储在_info成员变量中

    QPixmap pixmap(_info->_icon); //加载图片
    //设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->user_name_lb->setText(_info->_name); //设置用户名标签的文本为用户的名字
}

void ConUserItem::ShowRedPoint(bool show)
{
    //如果show参数为true, 则显示红点提示；否则隐藏红点提示
    if(show){
        ui->red_point->show();
    }else{
        ui->red_point->hide();
    }
}

std::shared_ptr<UserInfo> ConUserItem::GetInfo()
{
    return _info;
}
