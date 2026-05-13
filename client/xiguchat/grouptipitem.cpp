#include "grouptipitem.h"
#include "ui_grouptipitem.h"

GroupTipItem::GroupTipItem(QWidget *parent) : ListItemBase(parent),
    ui(new Ui::GroupTipItem), _tip("")
{
    ui->setupUi(this);
    SetItemType(ListItemType::GROUP_TIP_ITEM); //设置条目类型为分组提示
}

GroupTipItem::~GroupTipItem()
{
    delete ui;
}

QSize GroupTipItem::sizeHint() const
{
    return QSize(350, 40); //返回自定义的尺寸
}

void GroupTipItem::SetGroupTip(QString str)
{
    ui->label->setText(str); //设置分组提示文本
}
