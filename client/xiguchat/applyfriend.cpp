#include "applyfriend.h"
#include "ui_applyfriend.h"
#include "clickedlabel.h"
#include "friendlabel.h"
#include <QScrollBar>
#include "usermgr.h"
#include "tcpmgr.h"
#include <QJsonDocument>

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplyFriend), _label_point(5, 6)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint); //隐藏对话框标题栏
    this->setObjectName("ApplyFriend");
    this->setModal(true); //设置为模态对话框, 阻止用户在关闭对话框前与其他窗口进行交互
    ui->name_ed->setPlaceholderText(tr("恋恋风辰")); //设置申请备注输入框的占位文本
    ui->lb_ed->setPlaceholderText("搜索、添加标签"); //设置标签输入框的占位文本
    ui->back_ed->setPlaceholderText("燃烧的胸毛"); //设置好友备注输入框的占位文本

    ui->lb_ed->move(5, 5); //设置标签输入框的初始位置
    ui->lb_ed->setFixedHeight(40); //设置标签输入框的固定高度
    ui->lb_ed->setMaxLength(20); //设置标签输入框的最大输入长度为20个字符, 超过后将无法继续输入
    ui->input_tip_wid->hide(); //初始隐藏输入提示框

    _tip_cur_point = QPoint(5, 5); //设置标签展示区第一个标签的初始位置

    _tip_data = { "同学", "家人", "菜鸟教程", "C++ Primer", "Rust 程序设计",
                 "父与子学Python", "nodejs开发指南", "go 语言开发指南",
                 "游戏伙伴", "金融投资", "微信读书", "拼多多拼友" };

    //连接显示更多标签的标签点击事件到槽函数
    connect(ui->more_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::ShowMoreLabel);

    InitTipLbs(); //初始化提示标签, 将预设的标签添加到标签展示区, 并连接点击事件到槽函数

    //链接输入标签回车事件
    connect(ui->lb_ed, &CustomizeEdit::returnPressed, this, &ApplyFriend::SlotLabelEnter);
    //连接输入标签文本变化事件到槽函数, 用于根据输入的文本显示不同的输入提示
    connect(ui->lb_ed, &CustomizeEdit::textChanged, this, &ApplyFriend::SlotLabelTextChange);
    //连接提示标签点击事件到槽函数, 通过点击提示标签的内容来添加好友便签
    connect(ui->tip_lb, &ClickedOnceLabel::clicked, this, &ApplyFriend::SlotAddFirendLabelByClickTip);

    //隐藏滚动区域的滚动条, 通过事件过滤器在鼠标进入和离开时显示和隐藏垂直滚动条
    ui->scrollArea->horizontalScrollBar()->setHidden(true);
    ui->scrollArea->verticalScrollBar()->setHidden(true);
    ui->scrollArea->installEventFilter(this); //安装事件过滤器
    //设置确认和取消按钮的状态样式
    ui->sure_btn->SetState("normal", "hover", "press");
    ui->cancel_btn->SetState("normal", "hover", "press");

    //连接确认和取消按钮的槽函数
    connect(ui->cancel_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(ui->sure_btn, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend()
{
    qDebug()<< "ApplyFriend destruct";
    delete ui;
}

void ApplyFriend::InitTipLbs()
{
    int lines = 1; //当前行数, 初始为第一行
    for(int i = 0; i < _tip_data.size(); i++)
    {
        auto* lb = new ClickedLabel(ui->lb_list); //创建一个新的标签控件
        lb->SetState("normal", "hover", "pressed", "selected_normal",
            "selected_hover", "selected_pressed");
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]); //设置标签文本为预设的标签数据

        //连接标签的点击事件到槽函数, 通过点击标签实现增加和减少好友便签
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); //获取QLabel控件的字体信息
        int textWidth = fontMetrics.width(lb->text()); //获取文本的宽度
        int textHeight = fontMetrics.height(); //获取文本的高度

        //如果当前标签的x坐标加上文本宽度和间距超过标签展示区的宽度, 则换行显示
        if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width()) {
            lines++; //增加行数
            //默认最多显示两行标签, 超过两行则不显示后续标签
            if (lines > 2) {
                delete lb;
                return;
            }
            _tip_cur_point.setX(tip_offset); //换行后第一个标签的x坐标重置为间距值
            //换行后第一个标签的y坐标增加文本高度和间距值
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 20);
        }

        auto next_point = _tip_cur_point; //下一个标签的位置, 初始为当前标签的位置
        //将当前标签添加到标签展示区, 并更新下一个标签的位置
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point; //更新当前标签的位置为下一个标签的位置
    }
}

void ApplyFriend::AddTipLbs(ClickedLabel* lb, QPoint cur_point,
    QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point); //将标签移动到当前坐标位置
    lb->show(); //显示标签
    _add_labels.insert(lb->text(), lb); //将标签添加到已经创建好的标签的map中
    _add_label_keys.push_back(lb->text()); //将标签文本添加到已经创建好的标签的key列表中
    //更新下一个标签的x坐标为当前标签的x坐标加上文本宽度和间距值
    next_point.setX(lb->pos().x() + text_width + 20);
    next_point.setY(lb->pos().y()); //下一个标签的y坐标与当前标签相同
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    //如果事件对象是滚动区域, 并且事件类型是鼠标进入事件
    if (obj == ui->scrollArea && event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false); //显示垂直滚动条
    }
    //如果事件对象是滚动区域, 并且事件类型是鼠标离开事件
    else if (obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true); //隐藏垂直滚动条
    }
    //对于其他事件, 调用基类的事件过滤器处理
    return QObject::eventFilter(obj, event);
}

void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si; //保存搜索到的用户信息
    auto applyname = UserMgr::GetInstance()->GetName(); //获取当前用户的用户名作为好友申请的默认值
    auto bakname = si->_name; //获取搜索到的用户的昵称作为好友备注的默认值
    ui->name_ed->setText(applyname); //设置好友申请输入框的文本为当前用户的用户名
    ui->back_ed->setText(bakname); //设置好友备注输入框的文本为搜索到的用户的昵称
}

void ApplyFriend::ShowMoreLabel()
{
    qDebug()<< "receive more label clicked";
    ui->more_li_wid->hide(); //隐藏显示更多标签的标签和分割线

    ui->lb_list->setFixedWidth(520); //设置标签展示区的固定宽度, 以便重新排列标签
    _tip_cur_point = QPoint(5, 5); //重置标签展示区当前坐标, 从第一个标签位置重新排列
    auto next_point = _tip_cur_point; //下一个标签位置, 初始为第一个标签位置
    int textWidth; //文本宽度
    int textHeight; //文本高度

    //将已经添加的标签重新排列
    for(auto & added_key : _add_label_keys){
        auto added_lb = _add_labels[added_key]; //获取已经添加的标签控件

        QFontMetrics fontMetrics(added_lb->font()); // 获取QLabel控件的字体信息
        textWidth = fontMetrics.width(added_lb->text()); // 获取文本的宽度
        textHeight = fontMetrics.height(); // 获取文本的高度

        //如果当前标签的x坐标加上文本宽度和间距超过标签展示区的宽度, 则换行显示
        if(_tip_cur_point.x() +textWidth + tip_offset > ui->lb_list->width())
        {
            _tip_cur_point.setX(tip_offset); //换行后第一个标签的x坐标重置为间距值
            //换行后第一个标签的y坐标增加文本高度和间距值
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 20);
        }
        added_lb->move(_tip_cur_point); //将标签移动到当前坐标位置

        //更新下一个标签的x坐标为当前标签的x坐标加上文本宽度和间距值
        next_point.setX(added_lb->pos().x() + textWidth + 20);
        next_point.setY(_tip_cur_point.y()); //下一个标签的y坐标与当前标签相同

        _tip_cur_point = next_point; //更新当前标签的位置为下一个标签的位置, 为下一个标签的排列做准备

    }

    //添加未添加的
    for(int i = 0; i < _tip_data.size(); i++)
    {
        //判断预设的标签数据是否已经在标签展示区中存在, 如果存在则跳过, 不需要重新添加
        auto iter = _add_labels.find(_tip_data[i]);
        if(iter != _add_labels.end())
            continue;

        auto* lb = new ClickedLabel(ui->lb_list); //创建一个新的标签控件
        lb->SetState("normal", "hover", "pressed",
            "selected_normal", "selected_hover", "selected_pressed"); //设置标签的状态样式
        lb->setObjectName("tipslb"); //设置标签的对象名称
        lb->setText(_tip_data[i]); //设置标签文本为预设的标签数据

        //连接标签的点击事件到槽函数, 通过点击标签实现增加和减少好友便签
        connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
        int textHeight = fontMetrics.height(); // 获取文本的高度

        //如果当前标签的x坐标加上文本宽度和间距超过标签展示区的宽度, 则换行显示
        if (_tip_cur_point.x() + textWidth + tip_offset > ui->lb_list->width())
        {
            _tip_cur_point.setX(tip_offset); //换行后第一个标签的x坐标重置为间距值
            //换行后第一个标签的y坐标增加文本高度和间距值
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 20);
        }

        next_point = _tip_cur_point; //下一个标签的位置, 初始为当前标签的位置
        //将当前标签添加到标签展示区, 并更新下一个标签的位置
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point; //更新当前标签的位置为下一个标签的位置
    }

    //计算标签展示区需要增加的高度, 为新添加的标签腾出空间
    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    //设置标签展示区的固定高度为新标签的y坐标加上文本高度和间距值, 以适应新的标签显示
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
    //设置滚动区域内容的固定高度为当前高度加上增加的高度, 以适应新的标签显示
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void ApplyFriend::resetLabels()
{
    auto max_width = ui->gridWidget->width(); //获取标签展示区的宽度, 用于判断是否需要换行显示
    auto label_height = 0; //标签高度, 用于换行时更新y坐标
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++)
    {
        //如果当前标签的x坐标加上标签宽度超过标签展示区的宽度, 则换行显示
        if( _label_point.x() + iter.value()->width() > max_width)
        {
            //换行后第一个标签的y坐标增加标签高度和间距值
            _label_point.setY(_label_point.y() + iter.value()->height() + 6);
            _label_point.setX(2); //换行后第一个标签的x坐标重置为初始值
        }

        iter.value()->move(_label_point); //将标签移动到当前坐标位置
        iter.value()->show(); //显示标签

        //更新下一个标签的x坐标为当前标签的x坐标加上标签宽度和间距值
        _label_point.setX(_label_point.x()+iter.value()->width() + 5);
        _label_point.setY(_label_point.y()); //下一个标签的y坐标与当前标签相同
        label_height = iter.value()->height(); //更新标签高度为当前标签的高度, 为换行做准备
    }

    //如果标签展示区没有任何标签
    if(_friend_labels.isEmpty())
    {
        ui->lb_ed->move(_label_point); //将标签输入框移动到初始位置显示
        return;
    }

    //如果标签输入框的x坐标加上最小输入框长度超过标签展示区的宽度, 则将标签输入框移动到下一行显示
    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width())
    {
        //换行后标签输入框的x坐标重置为初始值, y坐标增加标签高度和间距值
        ui->lb_ed->move(5, _label_point.y() + label_height + 6);
    }
    else{
        ui->lb_ed->move(_label_point); //将标签输入框移动到当前标签的下一个位置显示
    }
}

void ApplyFriend::addLabel(QString name)
{
    //如果标签已经存在, 则不添加
    if (_friend_labels.find(name) != _friend_labels.end())
        return;

    auto tmplabel = new FriendLabel(ui->gridWidget); //创建一个新的好友标签控件
    tmplabel->SetText(name); //设置好友标签的文本为输入的标签文本
    tmplabel->setObjectName("FriendLabel");

    auto max_width = ui->gridWidget->width(); //获取标签展示区的宽度, 用于判断是否需要换行显示
    //如果当前标签的x坐标加上标签宽度超过标签展示区的宽度, 则换行显示
    if (_label_point.x() + tmplabel->width() > max_width)
    {
        //换行后第一个标签的y坐标增加标签高度和间距值
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(5); //换行后第一个标签的x坐标重置为初始值
    }

    tmplabel->move(_label_point); //将标签移动到当前坐标位置
    tmplabel->show(); //显示标签
    _friend_labels[tmplabel->Text()] = tmplabel; //将标签添加到好友标签的map中
    _friend_label_keys.push_back(tmplabel->Text()); //将标签文本添加到好友标签的key列表中

    //连接标签的关闭信号到槽函数
    connect(tmplabel, &FriendLabel::sig_close, this, &ApplyFriend::SlotRemoveFriendLabel);

    //更新下一个标签的x坐标为当前标签的x坐标加上标签宽度和间距值
    _label_point.setX(_label_point.x() + tmplabel->width() + 5);

    //如果标签输入框的x坐标加上最小输入框长度超过标签展示区的宽度, 则将标签输入框移动到下一行显示
    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidget->width())
    {
        //换行后标签输入框的x坐标重置为初始值, y坐标增加标签高度和间距值
        ui->lb_ed->move(5, _label_point.y() + tmplabel->height() + 4);
    }
    else {
        ui->lb_ed->move(_label_point); //将标签输入框移动到当前标签的下一个位置显示
    }

    ui->lb_ed->clear(); //清空标签输入框的文本

    //如果标签展示区的高度不足以显示当前标签和标签输入框, 则增加标签展示区的高度以适应新的标签
    if (ui->gridWidget->height() < _label_point.y() + tmplabel->height() + 4)
    {
        //增加标签展示区的高度为当前标签的y坐标加上标签高度的两倍和一些间距值
        ui->gridWidget->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 4);
        //增加滚动区域内容的高度以适应新的标签显示
        ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + tmplabel->height() * 2 + 4);
    }
}

void ApplyFriend::SlotLabelEnter()
{
    //如果标签输入框的文本为空, 则不添加标签, 直接返回
    if(ui->lb_ed->text().isEmpty())
        return;

    auto text = ui->lb_ed->text(); //获取标签输入框的文本
    addLabel(text); //添加标签到标签展示区
    ui->input_tip_wid->hide(); //隐藏输入提示框

    //判断输入的标签文本是否在预设的标签数据中存在
    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //如果没有找到
    if (find_it == _tip_data.end())
    {
        _tip_data.push_back(text); //将输入的标签文本添加到预设的标签数据中
    }

    //判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    //如果找到了
    if (find_add != _add_labels.end())
    {
        find_add.value()->SetCurState(ClickLbState::Selected); //将标签状态设置为选中状态
        return;
    }

    //标签展示栏也增加一个标签, 并设置绿色选中
    auto* lb = new ClickedLabel(ui->lb_list);
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("tipslb");
    lb->setText(text); //设置标签文本为输入的标签文本

    //连接标签的点击事件到槽函数, 通过点击标签实现增加和减少好友便签
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

    qDebug() << "ui->lb_list->width() is " << ui->lb_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度
    qDebug() << "textWidth is " << textWidth;

    //如果当前标签的x坐标加上文本宽度和间距超过标签展示区的宽度, 则换行显示
    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->lb_list->width())
    {
        _tip_cur_point.setX(5); //换行后第一个标签的x坐标重置为初始值
        //换行后第一个标签的y坐标增加文本高度和间距值
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 20);
    }

    auto next_point = _tip_cur_point; //下一个标签的位置, 初始为当前标签的位置

    //将当前标签添加到标签展示区, 并更新下一个标签的位置
    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point; //更新当前标签的位置为下一个标签的位置, 为下一个标签的排列做准备

    //计算标签展示区需要增加的高度, 为新添加的标签腾出空间
    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    //设置标签展示区的固定高度为新标签的y坐标加上文本高度和间距值, 以适应新的标签显示
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
    lb->SetCurState(ClickLbState::Selected); //将标签状态设置为选中状态
    //设置滚动区域内容的固定高度为当前高度加上增加的高度, 以适应新的标签显示
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "receive close signal";

    _label_point.setX(2); //重置标签展示区当前坐标的x坐标为初始值
    _label_point.setY(6); //重置标签展示区当前坐标的y坐标为初始值

    auto find_iter = _friend_labels.find(name); //在好友标签的map中查找要移除的标签
    //如果找不到要移除的标签, 则直接返回
    if(find_iter == _friend_labels.end())
        return;

    auto find_key = _friend_label_keys.end(); //定义一个迭代器
    //遍历好友标签的key列表
    for(auto iter = _friend_label_keys.begin(); iter != _friend_label_keys.end(); iter++){
        //如果找到了要移除的标签文本, 则保存迭代器并跳出循环
        if(*iter == name){
            find_key = iter;
            break;
        }
    }

    //如果找到了要移除的标签文本
    if(find_key != _friend_label_keys.end()){
        _friend_label_keys.erase(find_key); //从好友标签的key列表中移除要移除的标签文本
    }

    delete find_iter.value(); //删除要移除的标签控件
    _friend_labels.erase(find_iter); //从好友标签的map中移除要移除的标签

    resetLabels(); //重新排列剩余的标签

    auto find_add = _add_labels.find(name); //在已经创建好的标签的map中查找要移除的标签
    //如果找不到要移除的标签, 则直接返回
    if(find_add == _add_labels.end())
        return;

    find_add.value()->ResetNormalState(); //将要移除的标签的状态重置为正常状态
}

void ApplyFriend::SlotChangeFriendLabelByTip(QString lbtext, ClickLbState state)
{
    auto find_iter = _add_labels.find(lbtext); //判断点击的标签是否在标签展示栏中存在
    //如果不存在, 直接返回, 不进行任何操作
    if(find_iter == _add_labels.end())
        return;

    //如果标签状态是选中
    if(state == ClickLbState::Selected){
        addLabel(lbtext); //添加好友便签
        return;
    }
    //如果标签状态是正常
    if(state == ClickLbState::Normal){
        SlotRemoveFriendLabel(lbtext); //通过调用移除好友便签的槽函数实现取消选中状态
        return;
    }
}

void ApplyFriend::SlotLabelTextChange(const QString& text)
{
    //如果输入框文本为空
    if (text.isEmpty())
    {
        ui->tip_lb->setText(""); //清空提示标签的文本
        ui->input_tip_wid->hide(); //隐藏输入提示框
        return;
    }

    //判断输入的文本是否在预设的标签数据中存在
    auto iter = std::find(_tip_data.begin(), _tip_data.end(), text);
    //如果没有找到
    if (iter == _tip_data.end())
    {
        auto new_text = add_prefix + text; //将输入的文本添加前缀作为新的提示文本, 提示用户点击添加该标签
        ui->tip_lb->setText(new_text); //设置提示标签的文本为新的提示文本
        ui->input_tip_wid->show(); //显示输入提示框
        return;
    }
    ui->tip_lb->setText(text); //设置提示标签的文本为输入的文本, 提示用户点击该标签进行添加
    ui->input_tip_wid->show(); //显示输入提示框
}

void ApplyFriend::SlotAddFirendLabelByClickTip(QString text)
{
    int index = text.indexOf(add_prefix); //判断点击的提示标签文本是否包含前缀
    //如果包含前缀, 则将前缀去掉, 获取真正的标签文本
    if (index != -1){
        text = text.mid(index + add_prefix.length());
    }
    addLabel(text); //添加标签到标签展示区
    ui->input_tip_wid->hide(); //隐藏输入提示框

    //判断输入的标签文本是否在预设的标签数据中存在
    auto find_it = std::find(_tip_data.begin(), _tip_data.end(), text);
    //如果没有找到
    if (find_it == _tip_data.end()) {
        _tip_data.push_back(text); //将输入的标签文本添加到预设的标签数据中
    }

    //判断标签展示栏是否有该标签
    auto find_add = _add_labels.find(text);
    //如果找到了
    if (find_add != _add_labels.end()) {
        find_add.value()->SetCurState(ClickLbState::Selected); //将标签状态设置为选中状态
        return;
    }

    auto* lb = new ClickedLabel(ui->lb_list); //创建一个新的标签控件
    lb->SetState("normal", "hover", "pressed", "selected_normal",
        "selected_hover", "selected_pressed"); //设置标签的状态样式
    lb->setObjectName("tipslb");
    lb->setText(text); //设置标签文本为输入的标签文本

    //连接标签的点击事件到槽函数, 通过点击标签实现增加和减少好友便签
    connect(lb, &ClickedLabel::clicked, this, &ApplyFriend::SlotChangeFriendLabelByTip);

    qDebug() << "ui->lb_list->width() is " << ui->lb_list->width();
    qDebug() << "_tip_cur_point.x() is " << _tip_cur_point.x();

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度
    qDebug() << "textWidth is " << textWidth;

    //如果当前标签的x坐标加上文本宽度和间距超过标签展示区的宽度, 则换行显示
    if (_tip_cur_point.x() + textWidth + tip_offset + 3 > ui->lb_list->width())
    {
        _tip_cur_point.setX(5); //换行后第一个标签的x坐标重置为初始值
        //换行后第一个标签的y坐标增加文本高度和间距值
        _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 20);
    }

    auto next_point = _tip_cur_point; //下一个标签的位置, 初始为当前标签的位置

    //将当前标签添加到标签展示区, 并更新下一个标签的位置
    AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
    _tip_cur_point = next_point; //更新当前标签的位置为下一个标签的位置, 为下一个标签的排列做准备

    //计算标签展示区需要增加的高度, 为新添加的标签腾出空间
    int diff_height = next_point.y() + textHeight + tip_offset - ui->lb_list->height();
    //设置标签展示区的固定高度为新标签的y坐标加上文本高度和间距值, 以适应新的标签显示
    ui->lb_list->setFixedHeight(next_point.y() + textHeight + tip_offset);
    lb->SetCurState(ClickLbState::Selected); //将标签状态设置为选中状态
    //设置滚动区域内容的固定高度为当前高度加上增加的高度, 以适应新的标签显示
    ui->scrollcontent->setFixedHeight(ui->scrollcontent->height() + diff_height);
}

void ApplyFriend::SlotApplySure()
{
    //发送请求逻辑
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["uid"] = uid;
    auto name = ui->name_ed->text();
    if(name.isEmpty()){
        name = ui->name_ed->placeholderText();
    }

    jsonObj["applyname"] = name;

    auto bakname = ui->back_ed->text();
    if(bakname.isEmpty()){
        bakname = ui->back_ed->placeholderText();
    }

    jsonObj["bakname"] = bakname;
    jsonObj["touid"] = _si->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_ADD_FRIEND_REQ, jsonData);
    this->hide(); //隐藏对话框
    deleteLater(); //删除对话框对象, 释放资源
}

void ApplyFriend::SlotApplyCancel()
{
    this->hide(); //隐藏对话框
    deleteLater(); //删除对话框对象, 释放资源
}
