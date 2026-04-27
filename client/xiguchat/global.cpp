#include "global.h"

QString gate_url_prefix = ""; //初始化

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w); //取消样式表的应用，恢复到默认状态
    w->style()->polish(w); //重新应用样式表，使得界面元素根据新的样式进行更新
};

std::function<QString(QString)> xorString = [] (QString input) {
    QString result = input; //复制原始字符审, 以便进行修改
    int length = input.length(); //获取字符串的长度
    length = length % 255;
    for (int i = 0; i < length; ++i) {
    //每个字符进行异或操作
    //这里假设字符都是ASCII，因此直接转换为QChar
    result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(length)));
    }
    return result;
};
