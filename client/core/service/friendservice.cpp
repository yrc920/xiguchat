#include "friendservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "tcpmgr.h"
#include "usermgr.h"
#include "userdata.h"

FriendService::FriendService()
{
    //搜索结果由 TcpMgr 解析成 SearchInfo，这里只负责转成界面能用的字段
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this,
            [this](std::shared_ptr<SearchInfo> info) {
                if (!_searching)
                    return;
                _searching = false;
                if (!info || info->_uid <= 0) {
                    emit searchFailed(QStringLiteral("没有找到这个用户"));
                    return;
                }
                emit userFound(info->_uid, info->_name, info->_nick,
                               info->_desc, info->_icon, info->_sex);
            });
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_connection_closed,
            this, [this] { _searching = false; });
}

FriendService::~FriendService()
{
}

// 搜索用户：纯数字按 uid 查，其它按用户名查
bool FriendService::searchUser(const QString &query)
{
    const QString text = query.trimmed();
    if (text.isEmpty() || _searching)
        return false;
    _searching = true;

    QJsonObject body;
    body["uid"] = text; // 服务端按内容判断：纯数字当 uid，否则当用户名

    emit TcpMgr::GetInstance()->sig_send_data(
        ReqId::ID_SEARCH_USER_REQ,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
    return true;
}

// 发起好友申请：applyname 是自报家门，bakname 是给对方设的备注
void FriendService::applyFriend(int touid, const QString &applyName,
                                const QString &backName)
{
    const int myUid = UserMgr::GetInstance()->GetUid();

    QJsonObject body;
    body["uid"] = myUid;
    body["applyname"] = applyName;
    body["bakname"] = backName;
    body["touid"] = touid;

    emit TcpMgr::GetInstance()->sig_send_data(
        ReqId::ID_ADD_FRIEND_REQ,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
}

// 同意对方的申请：back 是给对方设置的备注名
void FriendService::authFriend(int touid, const QString &backName)
{
    const int myUid = UserMgr::GetInstance()->GetUid();

    QJsonObject body;
    body["fromuid"] = myUid;
    body["touid"] = touid;
    body["back"] = backName;

    emit TcpMgr::GetInstance()->sig_send_data(
        ReqId::ID_AUTH_FRIEND_REQ,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
}
