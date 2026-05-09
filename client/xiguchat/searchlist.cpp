#include "searchlist.h"
#include<QScrollBar>
#include "adduseritem.h"
//#include "invaliditem.h"
//#include "findsuccessdlg.h"
#include "tcpmgr.h"
#include "customizeedit.h"
//#include "findfaildlg.h"
#include "findsuccessdlg.h"

SearchList::SearchList(QWidget *parent) : QListWidget(parent),
    _find_dlg(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
    //默认隐藏滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->viewport()->installEventFilter(this); //安装事件过滤器
    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);

    addTipItem(); //添加条目
    //连接搜索条目
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if(_find_dlg){
        _find_dlg->hide();
        _find_dlg = nullptr;
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
    if(pending){
        _loadingDialog = new LoadingDlg(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = pending;
    }else{
        _loadingDialog->hide();
        _loadingDialog->deleteLater();
        _send_pending = pending;
    }
}


void SearchList::addTipItem()
{
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); // 获取自定义widget对象
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    if(itemType == ListItemType::ADD_USER_TIP_ITEM)
    {
        //todo ...
        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        auto si = std::make_shared<SearchInfo>(0, "llfc", "llfc", "hello , my friend!", 0);
        std::dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si); //设置搜索信息
        _find_dlg->show();

        // if (_send_pending) {
        //     return;
        // }
        // waitPending(true);
        // auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        // auto uid_str = search_edit->text();
        // //此处发送请求给server
        // QJsonObject jsonObj;
        // jsonObj["uid"] = uid_str;

        // QJsonDocument doc(jsonObj);
        // QString jsonString = doc.toJson(QJsonDocument::Indented);

        // //发送tcp请求给chat server
        // emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonString);
        return;
    }

    //清除弹出框
    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    // waitPending(false);
    // if (si == nullptr) {
    //     _find_dlg = std::make_shared<FindFailDlg>(this);
    // }else{
    //     _find_dlg = std::make_shared<FindSuccessDlg>(this);
    // }

    // _find_dlg->show();
}
