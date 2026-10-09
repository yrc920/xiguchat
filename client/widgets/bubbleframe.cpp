#include "bubbleframe.h"
#include <QPainter>
#include <QDebug>

const int WIDTH_SANJIAO  = 12;  //三角宽

BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent)
    : QFrame(parent)
    ,m_role(role)
    ,m_margin(12)
{
    m_pHLayout = new QHBoxLayout(); //创建一个水平布局管理器
    if(m_role == ChatRole::Self)
        //设置水平布局的内边距, 如果消息的发送者是自己, 则设置右边距为三角宽加上默认边距, 其他边距为默认边距
        m_pHLayout->setContentsMargins(m_margin, m_margin-3, WIDTH_SANJIAO + m_margin, m_margin-3);
    else
        //设置水平布局的内边距, 如果消息的发送者是对方, 则设置左边距为三角宽加上默认边距, 其他边距为默认边距
        m_pHLayout->setContentsMargins(WIDTH_SANJIAO + m_margin-3, m_margin, m_margin, m_margin-3);

    this->setLayout(m_pHLayout); //将当前气泡组件的布局设置为水平布局
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin); //这个宏用于告诉编译器这个参数没有被使用, 以避免编译器发出未使用参数的警告
    //m_margin = margin;
}

void BubbleFrame::setWidget(QWidget *w)
{
    //如果水平布局中已经有内容组件了, 就直接返回
    if(m_pHLayout->count() > 0)
        return;
    //否则将新的内容组件添加到水平布局中
    else{
        m_pHLayout->addWidget(w);
    }
}

void BubbleFrame::paintEvent(QPaintEvent *e)
{
    QPainter painter(this); //创建一个QPainter对象, 用于在当前气泡组件上进行绘制
    painter.setPen(Qt::NoPen); //设置绘制的笔为无, 这样就不会绘制边框线条

    if(m_role == ChatRole::Other)
    {
        QColor bk_color(Qt::white); //设置气泡背景颜色为白色
        painter.setBrush(QBrush(bk_color)); //设置绘制的刷子为这个颜色

        //定义一个矩形区域, 从气泡组件的左上角开始, 宽度为气泡组件的宽度减去三角宽, 高度为气泡组件的高度
        QRect bk_rect = QRect(WIDTH_SANJIAO, 0, this->width()-WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect, 5, 5); //绘制一个带有圆角的矩形, 圆角的半径为5像素

        //定义小三角的三个点的坐标
        QPointF points[3] = {
            QPointF(bk_rect.x(), 12),
            QPointF(bk_rect.x(), 12 + WIDTH_SANJIAO + 3),
            QPointF(bk_rect.x() - WIDTH_SANJIAO, 12),
        };
        painter.drawPolygon(points, 3); //绘制一个由这三个点组成的多边形, 也就是气泡的小三角
    }
    else
    {
        QColor bk_color(158,234,106); //设置气泡背景颜色为浅绿色
        painter.setBrush(QBrush(bk_color)); //设置绘制的刷子为这个颜色

        //定义一个矩形区域, 从气泡组件的左上角开始, 宽度为气泡组件的宽度减去三角宽, 高度为气泡组件的高度
        QRect bk_rect = QRect(0, 0, this->width()-WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect, 5, 5); //绘制一个带有圆角的矩形, 圆角的半径为5像素

        //定义小三角的三个点的坐标
        QPointF points[3] = {
            QPointF(bk_rect.x() + bk_rect.width(), 12),
            QPointF(bk_rect.x() + bk_rect.width(), 12 + WIDTH_SANJIAO + 3),
            QPointF(bk_rect.x() + bk_rect.width() + WIDTH_SANJIAO, 12),
        };
        painter.drawPolygon(points, 3); //绘制一个由这三个点组成的多边形, 也就是气泡的小三角
    }
    return QFrame::paintEvent(e); //调用父类的paintEvent函数, 以确保气泡组件能够正确地绘制
}
