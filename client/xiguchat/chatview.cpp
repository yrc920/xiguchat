#include "chatview.h"
#include <QScrollBar>
#include <QEvent>
#include <QDebug>
#include <QStyleOption>
#include <QPainter>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
    , isAppended(false)
{
    /* 窗口主布局为垂直布局, 其中包含一个滚动区域组件 */
    QVBoxLayout *pMainLayout = new QVBoxLayout(); //创建一个垂直布局管理器
    this->setLayout(pMainLayout); //将当前窗口的布局设置为垂直布局
    pMainLayout->setMargin(0); //设置布局管理器的边距为0, 这样布局中的组件就会紧贴窗口边缘
    m_pScrollArea = new QScrollArea(); //创建一个滚动区域组件
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea); //将滚动区域添加到主布局中, 这样滚动区域就会显示在窗口中

    /* 滚动区域的内容为一个widget, 布局为垂直布局, 用于排列聊天项 */
    QWidget *w = new QWidget(this); //创建一个widget作为滚动区域的内容
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true); //设置这个widget自动填充背景, 这样它就会显示背景颜色或背景图片等样式
    QVBoxLayout *pHLayout_1 = new QVBoxLayout(); //创建一个垂直布局管理器, 用于排列聊天项
    //添加一个占位widget, 用于将聊天项推到顶部, 这样新添加的聊天项就会显示在底部
    pHLayout_1->addWidget(new QWidget(), 100000);
    w->setLayout(pHLayout_1); //将这个widget的布局设置为垂直布局
    m_pScrollArea->setWidget(w); //将这个widget设置为滚动区域的内容

    //设置滚动区域的垂直滚动条策略为始终隐藏
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar(); //获取滚动区域的垂直滚动条对象
    //连接滚动条的范围改变信号到槽函数,
    //当滚动条的范围发生改变时(添加了新的聊天项), 就会调用槽函数来调整滚动位置以保持在底部
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    /* 滚动区域的布局为水平布局, 包含滚动区域的内容和滚动条 */
    QHBoxLayout *pHLayout_2 = new QHBoxLayout(); //创建一个水平布局管理器, 用于排列滚动区域的内容和滚动条
    //将滚动条添加到水平布局中, 并使其悬浮显示在右边
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setMargin(0); //设置水平布局的边距为0, 这样布局中的组件就会紧贴窗口边缘
    m_pScrollArea->setLayout(pHLayout_2); //将滚动区域的布局设置为水平布局, 这样滚动区域就会包含滚动条
    pVScrollBar->setHidden(true); //设置滚动条初始状态为隐藏

    //设置滚动区域的内容widget可调整大小, 这样当窗口大小改变时, 内容widget也会相应地调整大小
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this); //安装事件过滤器
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)
{
    //获取滚动区域内容widget的布局对象, 并将其转换为垂直布局类型
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    qDebug() << "vl->count() is " << vl->count();
    //将新的聊天项插入到布局中, 插入位置为布局中最后一个占位widget之前, 这样新添加的聊天项就会显示在底部
    vl->insertWidget(vl->count()-1, item);
    isAppended = true; //设置标志位, 表示已经添加了聊天项
}

void ChatView::prependChatItem(QWidget *item)
{

}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{

}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    //如果事件类型是鼠标悬浮进入并且事件对象是滚动区域
    if(e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        //检查滚动条的最大值是否为0, 如果是0则说明没有内容需要滚动, 就保持滚动条隐藏; 否则就显示滚动条
        m_pScrollArea->verticalScrollBar()->
            setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    //如果事件类型是鼠标悬浮离开并且事件对象是滚动区域, 就隐藏滚动条
    else if(e->type() == QEvent::Leave && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e); //对于其他事件, 调用基类的事件过滤器处理
}

void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; //创建一个QStyleOption对象
    opt.init(this); //初始化这个对象，使它包含当前widget的状态和属性
    QPainter p(this); //创建一个QPainter对象，并将当前widget作为绘制设备
    //调用当前widget的style来绘制基本元素(PE_Widget)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void ChatView::onVScrollBarMoved(int min, int max)
{
    if(isAppended) //添加item可能调用多次
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar(); //获取滚动区域的垂直滚动条对象
        //将滚动条的位置设置为最大值, 这样滚动区域就会滚动到底部以显示新添加的聊天项
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        //设置一个单次定时器, 在500毫秒后将isAppended标志位重置为false, 这样就不会在短时间内多次调整滚动位置
        QTimer::singleShot(500, [this](){ isAppended = false; });
    }
}

void ChatView::initStyleSheet()
{

}
