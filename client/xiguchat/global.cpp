#include "global.h"

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w); //取消样式表的应用，恢复到默认状态
    w->style()->polish(w); //重新应用样式表，使得界面元素根据新的样式进行更新
};
