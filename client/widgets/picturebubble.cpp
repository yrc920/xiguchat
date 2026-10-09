#include "picturebubble.h"
#include <QLabel>

#define PIC_MAX_WIDTH 300 //设置图片气泡的最大宽度为300像素
#define PIC_MAX_HEIGHT 180 //设置图片气泡的最大高度为180像素

PictureBubble::PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent)
    : BubbleFrame(role, parent)
{
    QLabel *lb = new QLabel(); //创建一个标签组件, 用于显示图片消息内容
    lb->setScaledContents(true); //设置标签的内容自动缩放以适应标签的大小
    //将传入的图片参数缩放到指定的最大宽度和最大高度, 同时保持图片的宽高比不变
    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT), Qt::KeepAspectRatio);
    lb->setPixmap(pix); //将缩放后的图片设置为标签的显示内容
    this->setWidget(lb); //将标签组件设置为当前图片气泡的内容组件

    int left_margin = this->layout()->contentsMargins().left(); //获取图片气泡的水平边距
    int right_margin = this->layout()->contentsMargins().right(); //获取图片气泡的水平边距
    int v_margin = this->layout()->contentsMargins().bottom(); //获取图片气泡的垂直边距
    //设置图片气泡的固定大小, 宽度为图片的宽度加上水平边距, 高度为图片的高度加上垂直边距的两倍
    setFixedSize(pix.width()+left_margin + right_margin, pix.height() + v_margin *2);
}
