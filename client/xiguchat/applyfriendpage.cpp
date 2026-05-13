#include "applyfriendpage.h"
#include "ui_applyfriendpage.h"
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QRandomGenerator>
#include "applyfrienditem.h"
//#include "authenfriend.h"
#include "applyfriend.h"
#include "tcpmgr.h"
#include "usermgr.h"

ApplyFriendPage::ApplyFriendPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyFriendPage)
{
    ui->setupUi(this);
    loadApplyList(); //加载好友申请列表

    //连接好友申请列表中的显示搜索界面的信号到ApplyFriendPage的显示搜索界面的信号
    connect(ui->apply_friend_list, &ApplyFriendList::sig_show_search,
        this, &ApplyFriendPage::sig_show_search);

    //接受tcp传递的authrsp信号处理
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ApplyFriendPage::slot_auth_rsp);
}

ApplyFriendPage::~ApplyFriendPage()
{
    delete ui;
}

void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply)
{
    //先模拟头像随机, 以后头像资源增加资源服务器后再显示
    int randomValue = QRandomGenerator::global()->bounded(100); //生成0到99之间的随机整数
    int head_i = randomValue % heads.size();
    auto* apply_item = new ApplyFriendItem(); //创建好友申请条目
    auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid,
        apply->_name, apply->_desc,heads[head_i], apply->_name, 0, 0); //将好友申请信息封装成ApplyInfo对象
    apply_item->SetInfo(apply_info); //设置好友申请条目的信息

    QListWidgetItem* item = new QListWidgetItem; //创建QListWidgetItem对象
    //qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(apply_item->sizeHint()); //设置item的大小为自定义widget的大小
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable); //设置item不可选中和不可用
    ui->apply_friend_list->insertItem(0, item); //将item插入到列表的顶部
    ui->apply_friend_list->setItemWidget(item, apply_item); //将自定义widget设置为item的显示内容
    apply_item->ShowAddBtn(true); //显示添加按钮

    //收到审核好友信号
    connect(apply_item, &ApplyFriendItem::sig_auth_friend,
        [this](std::shared_ptr<ApplyInfo> apply_info) {
        // auto* authFriend = new AuthenFriend(this);
        // authFriend->setModal(true);
        // authFriend->SetApplyInfo(apply_info);
        // authFriend->show();
    });
}

void ApplyFriendPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; //创建一个QStyleOption对象
    opt.init(this); //初始化这个对象，使它包含当前widget的状态和属性
    QPainter p(this); //创建一个QPainter对象, 并将当前widget作为绘制设备
    //调用当前widget的style来绘制基本元素(PE_Widget)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ApplyFriendPage::loadApplyList()
{
    //添加好友申请
    auto apply_list = UserMgr::GetInstance()->GetApplyList(); //从用户管理器获取好友申请列表
    for(auto &apply: apply_list){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int head_i = randomValue % heads.size();

        auto* apply_item = new ApplyFriendItem(); //创建好友申请条目
        apply->SetIcon(heads[head_i]); //设置好友申请的头像
        apply_item->SetInfo(apply); //设置好友申请条目的信息

        QListWidgetItem* item = new QListWidgetItem; //创建QListWidgetItem对象
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint()); //设置item的大小为自定义widget的大小
        //设置item不可选中和不可用
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        ui->apply_friend_list->insertItem(0, item); //将item插入到列表的顶部
        ui->apply_friend_list->setItemWidget(item, apply_item); //将自定义widget设置为item的显示内容

        if(apply->_status){
            apply_item->ShowAddBtn(false); //如果已经添加好友了, 就隐藏添加按钮, 显示已经添加的标签
        }else{
            apply_item->ShowAddBtn(true); //如果还没有添加好友, 就显示添加按钮, 隐藏已经添加的标签
            auto uid = apply_item->GetUid(); //获取申请人的用户ID
            _unauth_items[uid] = apply_item; //将申请人的用户ID和好友申请条目对象存储在_unauth_items哈希表中
        }

        //收到审核好友信号
        connect(apply_item, &ApplyFriendItem::sig_auth_friend,
            [this](std::shared_ptr<ApplyInfo> apply_info) {
            // auto* authFriend = new AuthenFriend(this);
            // authFriend->setModal(true);
            // authFriend->SetApplyInfo(apply_info);
            // authFriend->show();
        });
    }

    // 模拟假数据，创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); //生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *apply_item = new ApplyFriendItem(); //创建好友申请条目
        //将好友申请信息封装成ApplyInfo对象, 状态设置为1表示已经添加好友了
        auto apply = std::make_shared<ApplyInfo>(0, names[name_i], strs[str_i],
            heads[head_i], names[name_i], 0, 1);
        apply_item->SetInfo(apply); //设置好友申请条目的信息

        QListWidgetItem *item = new QListWidgetItem; //创建QListWidgetItem对象
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint()); //设置item的大小为自定义widget的大小
        //设置item不可选中和不可用
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        ui->apply_friend_list->addItem(item); //将item添加到列表的末尾
        ui->apply_friend_list->setItemWidget(item, apply_item); //将自定义widget设置为item的显示内容

        //收到审核好友信号
        connect(apply_item, &ApplyFriendItem::sig_auth_friend,
            [this](std::shared_ptr<ApplyInfo> apply_info){
            // auto *authFriend =  new AuthenFriend(this);
            // authFriend->setModal(true);
            // authFriend->SetApplyInfo(apply_info);
            // authFriend->show();
        });
    }
}

void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto uid = auth_rsp->_uid; //获取认证响应中的用户ID
    auto find_iter = _unauth_items.find(uid); //在_unauth_items哈希表中查找这个用户ID对应的好友申请条目对象
    //如果没有找到这个用户ID对应的好友申请条目对象
    if (find_iter == _unauth_items.end())
        return;

    find_iter->second->ShowAddBtn(false); //更新好友申请条目的显示状态, 显示已经添加的标签, 隐藏添加按钮
}
