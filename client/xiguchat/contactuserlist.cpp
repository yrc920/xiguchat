#include "contactuserlist.h"
#include "global.h"
#include "listitembase.h"
#include "grouptipitem.h"
#include "conuseritem.h"
#include <QRandomGenerator>
#include "tcpmgr.h"
#include "usermgr.h"
#include <QTimer>
#include <QCoreApplication>

ContactUserList::ContactUserList(QWidget *parent) : _add_friend_item(nullptr), _load_pending(false)
{
    Q_UNUSED(parent);
    //默认隐藏滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this); //安装事件过滤器

    addContactUserList(); //模拟从数据库或者后端传输过来的数据,进行列表加载

    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &ContactUserList::slot_item_clicked);

    //链接对端同意认证后通知的信号
    // connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend,this,
    //         &ContactUserList::slot_add_auth_firend);

    //链接自己点击同意认证后界面刷新
    // connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp,this,
    //         &ContactUserList::slot_auth_rsp);
}


void ContactUserList::ShowRedPoint(bool bshow /*= true*/)
{
    _add_friend_item->ShowRedPoint(bshow); //显示新的朋友申请的红点提示
}

void ContactUserList::addContactUserList()
{
    //加载分组提示条目(新的朋友)
    auto * groupTip = new GroupTipItem(); //创建分组提示条目
    groupTip->SetGroupTip(tr("新的朋友")); //设置分组提示条目的文本为"新的朋友"
    QListWidgetItem *item = new QListWidgetItem; //创建QListWidgetItem对象
    item->setSizeHint(groupTip->sizeHint()); //设置item的大小为自定义widget的大小
    this->addItem(item);  //将item添加到QListWidget中
    this->setItemWidget(item, groupTip); //将自定义widget设置为item的显示内容
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable); //设置item不可选中

    //加载新的朋友申请条目
    _add_friend_item = new ConUserItem(); //创建新的朋友申请条目
    _add_friend_item->setObjectName("new_friend_item");
    _add_friend_item->SetInfo(0, tr("新的朋友"), ":/res/add_friend.png"); //设置新的朋友申请条目的信息
    _add_friend_item->SetItemType(ListItemType::APPLY_FRIEND_ITEM); //设置新的朋友申请条目的类型为好友申请
    QListWidgetItem *add_item = new QListWidgetItem; //创建QListWidgetItem对象
    //qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    add_item->setSizeHint(_add_friend_item->sizeHint()); //设置item的大小为自定义widget的大小
    this->addItem(add_item); //将item添加到QListWidget中
    this->setItemWidget(add_item, _add_friend_item); //将自定义widget设置为item的显示内容
    this->setCurrentItem(add_item); //默认设置新的朋友申请条目被选中

    //加载分组提示条目(联系人)
    auto * groupCon = new GroupTipItem(); //创建分组提示条目
    groupCon->SetGroupTip(tr("联系人")); //设置分组提示条目的文本为"联系人"
    _groupitem = new QListWidgetItem; //创建QListWidgetItem对象
    _groupitem->setSizeHint(groupCon->sizeHint()); //设置item的大小为自定义widget的大小
    this->addItem(_groupitem); //将item添加到QListWidget中
    this->setItemWidget(_groupitem, groupCon); //将自定义widget设置为item的显示内容
    _groupitem->setFlags(_groupitem->flags() & ~Qt::ItemIsSelectable); //设置item不可选中

    // //加载后端发送过来的好友列表
    // auto con_list = UserMgr::GetInstance()->GetConListPerPage();
    // for(auto & con_ele : con_list){
    //     auto *con_user_wid = new ConUserItem();
    //     con_user_wid->SetInfo(con_ele->_uid,con_ele->_name, con_ele->_icon);
    //     QListWidgetItem *item = new QListWidgetItem;
    //     //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    //     item->setSizeHint(con_user_wid->sizeHint());
    //     this->addItem(item);
    //     this->setItemWidget(item, con_user_wid);
    // }

    // UserMgr::GetInstance()->UpdateContactLoadedCount();

    //模拟列表， 创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *con_user_wid = new ConUserItem(); //创建联系人用户条目
        con_user_wid->SetInfo(0, names[name_i], heads[head_i]); //设置联系人用户条目的信息
        QListWidgetItem *item = new QListWidgetItem; //创建QListWidgetItem对象
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(con_user_wid->sizeHint()); //设置item的大小为自定义widget的大小
        this->addItem(item); //将item添加到QListWidget中
        this->setItemWidget(item, con_user_wid); //将自定义widget设置为item的显示内容
    }
}

bool ContactUserList::eventFilter(QObject *watched, QEvent *event)
{
    //检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport())
    {
        if (event->type() == QEvent::Enter) {
            //鼠标悬浮, 显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            //鼠标离开, 隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    //检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event); //将事件转换为QWheelEvent类型
        int numDegrees = wheelEvent->angleDelta().y() / 8; //获取滚轮滚动的角度
        int numSteps = numDegrees / 15; //计算滚动步数

        //设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        //检查是否滚动到底部
        QScrollBar *scrollBar = this->verticalScrollBar(); //获取垂直滚动条对象
        int maxScrollValue = scrollBar->maximum(); //获取滚动条的最大值
        int currentValue = scrollBar->value(); //获取滚动条的当前值
        //int pageSize = 10; // 每页加载的联系人数量

        if (maxScrollValue - currentValue <= 0)
        {
            // auto b_loaded = UserMgr::GetInstance()->IsLoadChatFin();
            // if(b_loaded){
            //     return true;
            // }

            // if(_load_pending){
            //     return true;
            // }

            // _load_pending = true;

            // QTimer::singleShot(100, [this](){
            //     _load_pending = false;
            //     QCoreApplication::quit(); // 完成后退出应用程序
            // });
            // 滚动到底部，加载新的联系人
            qDebug()<<"load more contact user";
            //发送信号通知聊天界面加载更多聊天内容
            emit sig_loading_contact_user();
        }
        return true; // 停止事件传递
    }
    return QListWidget::eventFilter(watched, event); //对于其他事件, 调用基类的事件过滤器进行默认处理
}

void ContactUserList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); // 获取自定义widget对象
    //如果widget为空
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    //对自定义widget进行操作, 将item转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    //如果转化失败
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType(); //获取item的类型
    //如果item的类型是无效的或者分组提示条目, 则不进行任何操作
    if(itemType == ListItemType::INVALID_ITEM || itemType == ListItemType::GROUP_TIP_ITEM)
    {
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    //如果item的类型是好友申请条目
    if(itemType == ListItemType::APPLY_FRIEND_ITEM)
    {
        qDebug()<< "apply friend item clicked ";
        emit sig_switch_apply_friend_page(); //跳转到好友申请界面
        return;
    }

    //如果item的类型是联系人用户条目
    if(itemType == ListItemType::CONTACT_USER_ITEM)
    {
        qDebug()<< "contact user item clicked ";
        auto con_item = qobject_cast<ConUserItem*>(customItem); //将基类ListItemBase转化为ConUserItem
        auto user_info = con_item->GetInfo(); //获取联系人用户条目的信息
        emit sig_switch_friend_info_page(user_info); //跳转到好友信息界面, 并传递用户信息
        return;
    }
}

// void ContactUserList::slot_add_auth_firend(std::shared_ptr<AuthInfo> auth_info)
// {
//     qDebug() << "slot add auth friend ";
//     // 在 groupitem 之后插入新项
//     int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
//     int str_i = randomValue%strs.size();
//     int head_i = randomValue%heads.size();

//     auto *con_user_wid = new ConUserItem();
//     con_user_wid->SetInfo(auth_info);
//     QListWidgetItem *item = new QListWidgetItem;
//     //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//     item->setSizeHint(con_user_wid->sizeHint());

//     // 获取 groupitem 的索引
//     int index = this->row(_groupitem);
//     // 在 groupitem 之后插入新项
//     this->insertItem(index + 1, item);

//     this->setItemWidget(item, con_user_wid);

// }

// void ContactUserList::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
// {
//     qDebug() << "slot auth rsp called";

//     // 在 groupitem 之后插入新项
//     int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
//     int str_i = randomValue%strs.size();
//     int head_i = randomValue%heads.size();

//     auto *con_user_wid = new ConUserItem();
//     con_user_wid->SetInfo(auth_rsp->_uid ,auth_rsp->_name, heads[head_i]);
//     QListWidgetItem *item = new QListWidgetItem;
//     //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//     item->setSizeHint(con_user_wid->sizeHint());

//     // 获取 groupitem 的索引
//     int index = this->row(_groupitem);
//     // 在 groupitem 之后插入新项
//     this->insertItem(index + 1, item);

//     this->setItemWidget(item, con_user_wid);

// }
