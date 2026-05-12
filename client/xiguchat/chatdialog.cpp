#include "chatdialog.h"
#include "ui_chatdialog.h"
#include <QAction>
#include <QRandomGenerator>
#include "chatuserwid.h"
#include "loadingdlg.h"

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog), _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode), _b_loading(false)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press"); //设置添加好友按钮的状态样式
    ui->search_edit->SetMaxLength(15); //设置搜索框的最大长度为15字节
    ShowSearch(false); //初始状态不显示搜索框

    //创建一个搜索动作并设置图标
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    //将搜索动作添加到LineEdit的前面位置
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索")); //设置搜索框的占位文本为"搜索"

    //创建一个清除动作并设置图标
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png")); //使用透明图标作为初始图标
    //将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    //当需要显示清除图标时, 更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); //文本为空时，切换回透明图标
        }
    });

    //连接清除动作的触发信号到槽函数, 用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear(); //清除文本
        clearAction->setIcon(QIcon(":/res/close_transparent.png")); //清除文本后，切换回透明图标
        ui->search_edit->clearFocus(); //失去焦点
        ShowSearch(false); //清除按钮被按下则不显示搜索框
    });

    //连接加载信号和槽
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user,
        this, &ChatDialog::slot_loading_chat_user);

    addChatUserList(); //添加一些随机生成的聊天用户列表项作为测试用例

    //加载图片并缩放到label的大小
    QPixmap pixmap(":/res/head_1.jpg");
    QPixmap scaledPixmap = pixmap.scaled(ui->side_head_lb->size(), Qt::KeepAspectRatio);
    ui->side_head_lb->setPixmap(scaledPixmap); //将缩放后的图片设置到QLabel上
    ui->side_head_lb->setScaledContents(true); //设置QLabel自动缩放图片内容以适应大小

    //设置侧边栏聊天和联系人的状态样式, 包括正常、悬浮、按下和选中状态的样式
    ui->side_chat_lb->SetState("normal", "hover", "pressed",
        "selected_normal" ,"selected_hover" ,"selected_pressed");
    ui->side_contact_lb->SetState("normal", "hover", "pressed",
        "selected_normal" ,"selected_hover", "selected_pressed");
    ui->side_chat_lb->SetSelected(true); //初始状态将聊天标签设置为选中状态

    //将侧边栏的聊天和联系人标签添加到标签组列表中, 以便后续管理它们的状态
    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);

    //连接侧边栏聊天和联系人的点击信号到槽函数, 用于切换界面状态
    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    //链接搜索框输入变化
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

    //检测鼠标点击位置判断是否要清空搜索框
    this->installEventFilter(this); //安装事件过滤器
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::slot_loading_chat_user()
{
    //如果正在加载更多聊天用户, 则直接返回, 避免重复加载
    if(_b_loading)
        return;

    _b_loading = true; //设置正在加载的标志为true
    LoadingDlg *loadingDialog = new LoadingDlg(this); //创建一个加载对话框的实例
    loadingDialog->setModal(true); //设置对话框为模态, 阻止用户与其他窗口交互
    loadingDialog->show(); //显示加载对话框
    qDebug() << "add new data to list.....";
    addChatUserList();
    //加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false; //设置正在加载的标志为false, 允许再次加载更多聊天用户
}

void ChatDialog::ShowSearch(bool bsearch)
{
    //如果需要显示搜索框, 则隐藏聊天用户列表和联系人列表, 显示搜索结果列表, 并将界面模式设置为搜索模式
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }
    //如果不需要显示搜索框, 则根据当前界面状态显示聊天用户列表或联系人列表,
    //隐藏搜索结果列表, 并将界面模式设置为当前界面状态
    else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

void ChatDialog::slot_side_chat()
{
    qDebug() << "receive side chat clicked";
    ClearLabelState(ui->side_chat_lb); //设置聊天标签的状态, 将其他标签的状态清除
    ui->stackedWidget->setCurrentWidget(ui->chat_page); //切换到聊天页面
    _state = ChatUIMode::ChatMode; //设置界面状态为聊天模式
    ShowSearch(false); //不显示搜索框
}

void ChatDialog::slot_side_contact(){
    qDebug() << "receive side contact clicked";
    ClearLabelState(ui->side_contact_lb); //设置联系标签的状态, 将其他标签的状态清除
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page); //切换到联系人页面
    _state = ChatUIMode::ContactMode; //设置界面状态为联系人模式
    ShowSearch(false); //不显示搜索框
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    //遍历标签列表, 跳过传入的标签
    for(auto & ele: _lb_list){
        if(ele == lb)
            continue;

        ele->ClearState(); //清除其他标签的状态, 只保留当前点击的标签的状态
    }
}

void ChatDialog::slot_text_changed(const QString &str)
{
    //如果搜索框文本不为空, 则显示搜索框
    if (!str.isEmpty()) {
        ShowSearch(true);
    }
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    //如果事件类型是鼠标按下事件
    if (event->type() == QEvent::MouseButtonPress) {
        //将事件转换为鼠标事件以获取鼠标点击的信息
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent); //调用处理全局鼠标点击事件的函数, 用于判断是否需要清空搜索框
    }
    return QDialog::eventFilter(watched, event); //调用基类的事件过滤器以确保正常的事件处理
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    //实现点击位置的判断和处理逻辑
    //先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if( _mode != ChatUIMode::SearchMode){
        return;
    }

    //将鼠标点击位置转换为搜索列表坐标系中的位置
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    //判断点击位置是否在聊天列表的范围内
    if (!ui->search_list->rect().contains(posInSearchList)) {
        ui->search_edit->clear(); //如果不在聊天列表内，清空输入框
        ShowSearch(false); //不显示搜索框
    }
}

void ChatDialog::addChatUserList()
{
    //创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); //生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid(); //创建一个ChatUserWid的实例
        //设置用户信息, 包括用户名、头像和消息
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint()); //设置列表项的大小为ChatUserWid的推荐大小
        ui->chat_user_list->addItem(item); //将列表项添加到聊天用户列表中
        //将ChatUserWid设置为列表项的widget, 这样列表项就会显示ChatUserWid的内容
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
    }
}
