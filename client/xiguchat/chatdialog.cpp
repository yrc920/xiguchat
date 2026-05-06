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
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    addChatUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
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

//测试用例: 添加一些随机生成的聊天用户列表项
std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

std::vector<QString> names = {
    "llfc",
    "zack",
    "golang",
    "cpp",
    "java",
    "nodejs",
    "python",
    "rust"
};

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
