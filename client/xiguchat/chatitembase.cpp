#include "chatitembase.h"
#include <QFont>
#include <QVBoxLayout>

ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent)
    : QWidget(parent)
    , m_role(role)
{
    //初始化显示用户名的标签
    m_pNameLabel = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei");
    font.setPointSize(8);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(30); //设置用户名标签的高度

    //初始化显示用户头像的标签
    m_pIconLabel = new QLabel();
    m_pIconLabel->setScaledContents(true); //设置头像标签的内容自动缩放以适应标签的大小
    m_pIconLabel->setFixedSize(70, 70);

    //初始化显示消息内容的气泡组件
    m_pBubble = new QWidget();
    QGridLayout *pGLayout = new QGridLayout(); //创建一个网格布局管理器
    pGLayout->setVerticalSpacing(7); //设置网格布局的垂直间距
    pGLayout->setHorizontalSpacing(7); //设置网格布局的水平间距
    pGLayout->setMargin(7); //设置网格布局的内边距
    //创建一个水平扩展的占位符, 用于调整布局中的空白区域
    QSpacerItem*pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    //如果消息的发送者是自己
    if(m_role == ChatRole::Self)
    {
        m_pNameLabel->setContentsMargins(0, 0, 12, 0); //设置用户名标签的右边距为12像素, 其他边距为0
        m_pNameLabel->setAlignment(Qt::AlignRight); //设置用户名标签的文本对齐方式为右对齐
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1); //将用户名标签添加到网格布局的第0行第1列, 占据1行1列
        //将头像标签添加到网格布局的第0行第2列, 占据2行1列, 对齐方式为顶部对齐
        pGLayout->addWidget(m_pIconLabel, 0, 2, 2, 1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1); //将水平扩展的占位符添加到网格布局的第1行第0列, 占据1行1列
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1); //将气泡组件添加到网格布局的第1行第1列, 占据1行1列
        pGLayout->setColumnStretch(0, 2); //设置弹簧的拉伸因子为2
        pGLayout->setColumnStretch(1, 3); //设置气泡组件的拉伸因子为3, 使其占据更多的空间
    }
    //如果消息的发送者是对方
    else{
        m_pNameLabel->setContentsMargins(12, 0, 0, 0); //设置用户名标签的左边距为12像素, 其他边距为0
        m_pNameLabel->setAlignment(Qt::AlignLeft); //设置用户名标签的文本对齐方式为左对齐
        //将头像标签添加到网格布局的第0行第0列, 占据2行1列, 对齐方式为顶部对齐
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1); //将用户名标签添加到网格布局的第0行第1列, 占据1行1列
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1); //将气泡组件添加到网格布局的第1行第1列, 占据1行1列
        pGLayout->addItem(pSpacer, 1, 2, 1, 1); //将水平扩展的占位符添加到网格布局的第1行第2列, 占据1行1列
        pGLayout->setColumnStretch(1, 3); //设置气泡组件的拉伸因子为3, 使其占据更多的空间
        pGLayout->setColumnStretch(2, 2); //设置弹簧的拉伸因子为2
    }
    this->setLayout(pGLayout); //将网格布局设置为当前聊天消息项的布局
}

void ChatItemBase::setUserName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w)
{
    //获取当前聊天消息项的布局管理器, 并将其转换为QGridLayout类型
    QGridLayout *pGLayout = (qobject_cast<QGridLayout *>)(this->layout());
    pGLayout->replaceWidget(m_pBubble, w); //将原来的气泡组件替换为新的组件w
    delete m_pBubble; //删除原来的气泡组件, 释放内存
    m_pBubble = w; //将新的组件w赋值给m_pBubble, 使其成为当前聊天消息项的气泡组件
}
