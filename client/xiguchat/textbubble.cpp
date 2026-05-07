#include "textbubble.h"
#include <QFontMetricsF>
#include <QDebug>
#include <QFont>
#include "global.h"
#include <QTimer>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QFont>

TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    : BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit(); //创建一个文本编辑区组件
    m_pTextEdit->setReadOnly(true); //设置文本编辑区为只读, 这样用户就不能修改文本内容
    //设置文本编辑区的滚动条策略为始终隐藏
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->installEventFilter(this);
    //设置文本编辑区的字体为微软雅黑, 字体大小为12
    QFont font("Microsoft YaHei");
    font.setPointSize(10);
    m_pTextEdit->setFont(font);

    setPlainText(text); //设置文本编辑区的纯文本内容为传入的文本参数
    setWidget(m_pTextEdit); //将文本编辑区组件设置为气泡组件的内容组件
    initStyleSheet(); //初始化文本气泡的样式表
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    //如果事件发生在文本编辑区组件上, 并且事件类型是绘制事件, 就调用调整文本气泡高度的函数
    if(m_pTextEdit == o && e->type() == QEvent::Paint)
    {
        adjustTextHeight(); //PaintEvent中设置
    }
    return BubbleFrame::eventFilter(o, e); //调用父类的eventFilter函数, 以确保其他事件能够正常处理
}

void TextBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text); //设置文本编辑区的纯文本内容为传入的文本参数

    //找到段落中最大宽度
    qreal doc_margin = m_pTextEdit->document()->documentMargin(); //文本编辑区的文档边距
    int margin_left = this->layout()->contentsMargins().left(); //文本编辑区的水平边距
    int margin_right = this->layout()->contentsMargins().right(); //文本编辑区的水平边距
    //创建一个QFontMetricsF对象, 用于测量文本编辑区当前字体的字符宽度和行距等信息
    QFontMetricsF fm(m_pTextEdit->font());
    QTextDocument *doc = m_pTextEdit->document(); //获取文本编辑区的文档对象
    int max_width = 0;
    //遍历每一段找到最宽的那一段
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        int txtW = int(fm.width(it.text())); //测量当前段落的文本宽度
        max_width = max_width < txtW ? txtW : max_width; //找到最长的那段
    }
    //设置这个气泡的最大宽度, 只需要设置一次
    setMaximumWidth(max_width + doc_margin * 2 + (margin_left + margin_right));
}

void TextBubble::adjustTextHeight()
{
    //文本编辑区的文档边距, 这个值是文本内容与文本编辑区边框之间的距离, 默认为4像素
    qreal doc_margin = m_pTextEdit->document()->documentMargin();
    QTextDocument *doc = m_pTextEdit->document(); //获取文本编辑区的文档对象

    qreal text_height = 0; //文本内容的高度
    //把每一段的高度相加得到文本内容的总高度,
    //这样就可以根据文本内容的高度来调整文本气泡的高度, 使其能够适应文本内容的显示
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        QTextLayout *pLayout = it.layout();
        QRectF text_rect = pLayout->boundingRect();
        text_height += text_rect.height();
    }

    //文本编辑区的垂直边距, 这个值是文本编辑区内容与气泡组件边框之间的距离, 默认为7像素
    int vMargin = this->layout()->contentsMargins().top();
    //设置这个气泡需要的高度 文本高+文本边距+TextEdit边框到气泡边框的距离
    setFixedHeight(text_height + doc_margin *2 + vMargin*2);
}

void TextBubble::initStyleSheet()
{
    //设置文本编辑区的样式表, 使其背景透明, 没有边框线条
    m_pTextEdit->setStyleSheet("QTextEdit{background:transparent;border:none}");
}
