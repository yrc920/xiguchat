#include "searchlist.h"
#include<QScrollBar>
#include "adduseritem.h"
//#include "invaliditem.h"
#include "tcpmgr.h"
#include "customizeedit.h"
#include "findfaildlg.h"
#include "findsuccessdlg.h"
#include <QJsonDocument>

SearchList::SearchList(QWidget *parent) : QListWidget(parent),
    _send_pending(false), _find_dlg(nullptr), _search_edit(nullptr)
{
    Q_UNUSED(parent);
    //默认隐藏滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this); //安装事件过滤器
    addTipItem(); //添加条目

    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);

    //连接搜索条目
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    //如果搜索结果对话框存在
    if(_find_dlg){
        _find_dlg->hide(); //隐藏搜索结果对话框
        _find_dlg = nullptr; //将搜索结果对话框的智能指针置空, 以释放资源
    }
}

void SearchList::SetSearchEdit(QWidget* edit) {
    _search_edit = edit;
}

bool SearchList::eventFilter(QObject *watched, QEvent *event)
{
    //检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            //鼠标悬浮，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            //鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    //检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        //将事件转换为QWheelEvent以获取滚轮滚动的信息
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8; //获取滚轮滚动的角度
        int numSteps = numDegrees / 15; //计算滚动步数

        //设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
        return true; //停止事件传递
    }
    return QListWidget::eventFilter(watched, event); //调用基类的事件过滤器以确保正常的事件处理
}

void SearchList::waitPending(bool pending)
{
    //如果正在等待搜索请求
    if(pending){
        _loadingDialog = new LoadingDlg(this); //创建加载对话框的实例
        _loadingDialog->setModal(true); //设置对话框为模态, 阻止用户与其他窗口交互
        _loadingDialog->show(); //显示加载对话框
        _send_pending = pending; //设置正在发送搜索请求的标志为true, 以防止重复发送搜索请求
    }
    //如果不再等待搜索请求
    else{
        _loadingDialog->hide(); //关闭加载对话框
        _loadingDialog->deleteLater(); //删除加载对话框的实例
        _send_pending = pending; //设置正在发送搜索请求的标志为false, 以允许再次发送搜索请求
    }
}

void SearchList::addTipItem()
{
    auto *invalid_item = new QWidget(); //创建一个无效条目的widget实例
    QListWidgetItem *item_tmp = new QListWidgetItem; //创建一个QListWidgetItem实例
    item_tmp->setSizeHint(QSize(400, 20)); //设置列表项的大小为一个较小的尺寸, 以适应搜索提示信息的显示
    this->addItem(item_tmp); //将列表项添加到搜索结果列表中
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item); //将无效条目的widget设置为列表项的widget
    //设置列表项不可选中, 以防止用户点击无效条目时触发点击事件
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);

    auto *add_user_item = new AddUserItem(); //创建一个添加用户条目的widget实例
    QListWidgetItem *item = new QListWidgetItem; //创建一个QListWidgetItem实例
    //设置列表项的大小为添加用户条目的推荐大小, 以适应添加用户提示项的显示
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item); //将列表项添加到搜索结果列表中
    this->setItemWidget(item, add_user_item); //将添加用户条目的widget设置为列表项的widget
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); //获取自定义widget对象
    //如果获取的widget对象为空, 则直接返回
    if(!widget){
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    //对自定义widget进行操作, 将item转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType(); //获取列表项的类型, 以便根据不同类型的列表项执行不同的操作
    //如果列表项类型无效, 则直接返回, 不执行任何操作
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    //如果列表项类型是添加用户提示项, 则执行添加用户的操作, 如弹出搜索结果对话框等
    if(itemType == ListItemType::ADD_USER_TIP_ITEM)
    {
        if (_send_pending) {
            return;
        }
        waitPending(true);
        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto uid_str = search_edit->text();
        //此处发送请求给server
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
        return;
    }

    CloseFindDlg(); //清除弹出框
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);
    if (si == nullptr) {
        _find_dlg = std::make_shared<FindFailDlg>(this);
    }else{
        //此处分两种情况，一种是搜多到已经是自己的朋友了，一种是未添加好友
        //查找是否已经是好友
        // bool bExist = UserMgr::GetInstance()->CheckFriendById(si->_uid);
        // if(bExist){
        //     //此处处理已经添加的好友，实现页面跳转
        //     //跳转到聊天界面指定的item中
        //     emit sig_jump_chat_item(si);
        //     return;
        // }
        //此处先处理为添加的好友
        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        std::dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si);
    }
    _find_dlg->show();
}
