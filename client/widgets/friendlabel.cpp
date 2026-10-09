#include "friendlabel.h"
#include "ui_friendlabel.h"
#include <QDebug>

FriendLabel::FriendLabel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FriendLabel)
{
    ui->setupUi(this);
    ui->close_lb->SetState("normal", "hover", "pressed",
        "selected_normal", "selected_hover", "selected_pressed"); //设置关闭按钮的状态样式

    //连接关闭按钮的点击信号到槽函数
    connect(ui->close_lb, &ClickedLabel::clicked, this, &FriendLabel::slot_close);
}

FriendLabel::~FriendLabel()
{
    delete ui;
}

void FriendLabel::SetText(QString text)
{
    _text = text;
    ui->tip_lb->setText(_text); //设置标签文本
    ui->tip_lb->adjustSize(); //调整标签大小以适应文本内容

    QFontMetrics fontMetrics(ui->tip_lb->font()); //获取QLabel控件的字体信息
    auto textWidth = fontMetrics.width(ui->tip_lb->text()); //获取文本的宽度
    auto textHeight = fontMetrics.height(); //获取文本的高度

    qDebug() << " ui->tip_lb.width() is " << ui->tip_lb->width();
    qDebug() << " ui->close_lb->width() is " << ui->close_lb->width();
    qDebug() << " textWidth is " << textWidth;
    //设置标签的宽度为文本宽度加上关闭按钮的宽度和一些间距
    this->setFixedWidth(ui->tip_lb->width() + ui->close_lb->width() + 5);
    this->setFixedHeight(textHeight + 12); //设置标签的高度为文本高度加上一些间距
    qDebug() << " this->setFixedHeight " << this->height();
    _width = this->width(); //保存标签的宽度
    _height = this->height(); //保存标签的高度
}

int FriendLabel::Width()
{
    return _width;
}

int FriendLabel::Height()
{
    return _height;
}

QString FriendLabel::Text()
{
    return _text;
}

void FriendLabel::slot_close()
{
    emit sig_close(_text); //发出关闭信号, 将标签文本作为参数传递给槽函数
}
