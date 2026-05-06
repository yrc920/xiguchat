#include "customizeedit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent) : QLineEdit(parent), _max_len(0)
{
    //连接文本改变信号和限制文本长度的槽函数
    connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::limitTextLength);
}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent *event)
{
    //执行失去焦点时的处理逻辑
    //qDebug() << "CustomizeEdit focusout";
    //调用基类的focusOutEvent()方法，保证基类的行为得到执行
    QLineEdit::focusOutEvent(event);

    emit sig_foucus_out(); //发送失去焦点得信号
}

void CustomizeEdit::limitTextLength(QString text)
{
    if(_max_len <= 0)
        return;

    QByteArray byteArray = text.toUtf8(); //将文本转换为UTF-8编码的字节数组

    //如果字节数组的长度超过最大长度
    if (byteArray.size() > _max_len) {
        byteArray = byteArray.left(_max_len); //截取前_max_len个字节
        this->setText(QString::fromUtf8(byteArray)); //将截取后的字节数组转换回字符串并设置为文本
    }
}
