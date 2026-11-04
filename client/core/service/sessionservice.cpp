#include "sessionservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "tcpmgr.h"
#include "usermgr.h"

namespace {
constexpr int kSessionTimeoutMs = 12000; // 连接 + 登录帧回包的整体上限
}

SessionService::SessionService() : _ready(false)
{
    _timeout.setSingleShot(true);
    _timeout.setInterval(kSessionTimeoutMs);
    connect(&_timeout, &QTimer::timeout, this, [this] {
        failAndReset(QStringLiteral("连接聊天服务超时"));
    });

    //服务端按心跳判活，断掉就踢下线；它属于会话而不是某个页面，
    //所以放在这里，切页面不影响保活
    _heartbeat.setInterval(10000);
    connect(&_heartbeat, &QTimer::timeout, this, [this] {
        if (!_ready)
            return;
        // 服务端按 fromuid 记录会话活跃时间，空包会被记成 uid 0
        QJsonObject frame;
        frame["fromuid"] = _current.Uid;
        emit TcpMgr::GetInstance()->sig_send_data(
            ReqId::ID_HEART_BEAT_REQ,
            QJsonDocument(frame).toJson(QJsonDocument::Indented));
    });

    auto *tcp = TcpMgr::GetInstance().get();

    // 收到新消息：写进 UserMgr。好友模型只刷新那一行的摘要，
    // 消息模型在同一个好友下会增量插入，界面不需要重建
    connect(tcp, &TcpMgr::sig_text_chat_msg, this,
            [](std::shared_ptr<TextChatMsg> msg) {
                if (!msg)
                    return;
                const int peerUid = msg->_from_uid == UserMgr::GetInstance()->GetUid()
                    ? msg->_to_uid : msg->_from_uid;
                if (msg->_thread_id > 0)
                    UserMgr::GetInstance()->SetThreadId(peerUid, msg->_thread_id);
                UserMgr::GetInstance()->AppendFriendChatMsg(peerUid, msg->_chat_msgs);
            });
    connect(tcp, &TcpMgr::sig_chat_msg_failed, this, [](const QJsonObject &data) {
        auto *mgr = UserMgr::GetInstance().get();
        const int peerUid = data.value("touid").toInt();
        for (const auto &value : data.value("text_array").toArray())
            mgr->UpdateChatMessage(peerUid, value.toObject().value("msgid").toString(), 0,
                ChatMessageStatus::Failed);
    });
    connect(tcp, &TcpMgr::sig_chat_threads, this, [this](const QJsonObject &data) {
        auto *mgr = UserMgr::GetInstance().get();
        const auto threads = data.value("thread_list").toArray();
        mgr->AppendChatThreads(threads);
        mgr->RefreshFriendList();
        for (const auto &value : threads) {
            const auto item = value.toObject();
            QJsonObject request{{"fromuid", mgr->GetUid()}, {"thread_id", item.value("thread_id")},
                                {"message_id", 0}};
            emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ,
                QJsonDocument(request).toJson(QJsonDocument::Compact));
        }
        if (data.value("load_more").toBool() && !threads.isEmpty()) {
            QJsonObject request{{"fromuid", mgr->GetUid()},
                {"thread_id", threads.last().toObject().value("thread_id")}};
            emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ,
                QJsonDocument(request).toJson(QJsonDocument::Compact));
        }
    });
    connect(tcp, &TcpMgr::sig_chat_messages, this, [](const QJsonObject &data) {
        auto *mgr = UserMgr::GetInstance().get();
        const qint64 threadId = data.value("thread_id").toVariant().toLongLong();
        const int peerUid = mgr->FriendIdByThread(threadId);
        if (peerUid <= 0)
            return;
        std::vector<std::shared_ptr<TextChatData>> messages;
        for (const auto &value : data.value("text_array").toArray()) {
            const auto item = value.toObject();
            const int defaultStatus = item.value("fromuid").toInt() == mgr->GetUid()
                ? static_cast<int>(ChatMessageStatus::Sent)
                : static_cast<int>(ChatMessageStatus::Unread);
            const auto status = static_cast<ChatMessageStatus>(
                item.value("status").toInt(defaultStatus));
            messages.push_back(std::make_shared<TextChatData>(item.value("msgid").toString(),
                item.value("content").toString(), item.value("fromuid").toInt(),
                item.value("touid").toInt(), item.value("message_id").toVariant().toLongLong(),
                threadId, status));
        }
        mgr->AppendFriendChatMsg(peerUid, std::move(messages));
        if (data.value("load_more").toBool()) {
            const auto items = data.value("text_array").toArray();
            if (!items.isEmpty()) {
                QJsonObject request{{"fromuid", mgr->GetUid()}, {"thread_id", threadId},
                    {"message_id", items.last().toObject().value("message_id")}};
                emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ,
                    QJsonDocument(request).toJson(QJsonDocument::Compact));
            }
        }
    });
    connect(tcp, &TcpMgr::sig_private_chat_created, this, [](const QJsonObject &data) {
        if (data.value("error").toInt() != 0) {
            UserMgr::GetInstance()->MarkPrivateChatPending(data.value("uid").toInt(), false);
            return;
        }
        UserMgr::GetInstance()->SetThreadId(data.value("uid").toInt(),
            data.value("thread_id").toVariant().toLongLong());
    });

    // 收到新的好友申请：同一个人只留一条，重复通知直接丢掉
    connect(tcp, &TcpMgr::sig_friend_apply, this,
            [](std::shared_ptr<AddFriendApply> apply) {
                if (!apply)
                    return;
                if (UserMgr::GetInstance()->AlreadyApply(apply->_from_uid))
                    return;
                UserMgr::GetInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
            });

    // 互相通过验证：把对方加进好友列表。两个信号分别对应
    // 「别人同意了我」和「我同意了别人」，都要落到同一份好友数据里
    connect(tcp, &TcpMgr::sig_add_auth_friend, this,
            [](std::shared_ptr<AuthInfo> info) {
                if (!info)
                    return;
                auto *mgr = UserMgr::GetInstance().get();
                if (!mgr->CheckFriendById(info->_uid))
                    mgr->AddFriend(info);
                mgr->SetThreadId(info->_uid, info->_thread_id);
                if (!info->_welcome_msgs.empty())
                    mgr->AppendFriendChatMsg(info->_uid, info->_welcome_msgs);
            });
    connect(tcp, &TcpMgr::sig_auth_rsp, this, [](std::shared_ptr<AuthRsp> rsp) {
        if (!rsp)
            return;
        // 服务端确认通过后，申请条目从「添加」变成「已添加」
        UserMgr::GetInstance()->SetApplyStatus(rsp->_uid, 1);
        auto *mgr = UserMgr::GetInstance().get();
        if (!mgr->CheckFriendById(rsp->_uid))
            mgr->AddFriend(rsp);
        mgr->SetThreadId(rsp->_uid, rsp->_thread_id);
        if (!rsp->_welcome_msgs.empty())
            mgr->AppendFriendChatMsg(rsp->_uid, rsp->_welcome_msgs);
    });
    connect(tcp, &TcpMgr::sig_con_success, this, &SessionService::onTcpConnected);
    connect(tcp, &TcpMgr::sig_login_failed, this, &SessionService::onChatLoginFailed);
    connect(tcp, &TcpMgr::sig_login_success, this, &SessionService::onChatLoggedIn);
    connect(tcp, &TcpMgr::sig_con_failed, this, [this](const QString &message) {
        if (!_connecting && !_ready)
            return;
        failAndReset(QStringLiteral("无法连接聊天服务器：%1").arg(message));
    });
    connect(tcp, &TcpMgr::sig_connection_closed, this, [this] {
        if (_connecting)
            failAndReset(QStringLiteral("聊天服务器在登录完成前断开了连接"));
        else {
            _ready = false;
            _heartbeat.stop();
            UserMgr::GetInstance()->FailPendingMessages();
        }
    });
    connect(tcp, &TcpMgr::sig_notify_offline, this, [this] {
        if (_connecting) {
            failAndReset(QStringLiteral("登录期间会话被其他终端替换，请重试"));
            return;
        }
        _ready = false;
        _heartbeat.stop();
    });
}

SessionService::~SessionService()
{
}

// 建立会话：清掉上一个账号的数据，连 TCP 并发登录帧，整体超时兜底
void SessionService::connectToChat(const ServerInfo &info)
{
    closeSession();
    // 每次登录都从空的用户数据开始
    UserMgr::GetInstance()->ClearSession();

    _pending = info;
    _connecting = true;
    _ready = false;
    _timeout.start();

    emit connecting();

    // 同线程直接调用 TcpMgr 的槽：它的 socket 是私有的，
    // 这里只表达“请连到这台服务器”，不参与 socket 生命周期
    TcpMgr::GetInstance()->slot_tcp_connect(info);
}

// 主动结束会话：停心跳、停超时、断开连接
void SessionService::closeSession()
{
    _timeout.stop();
    _heartbeat.stop();
    _ready = false;
    _connecting = false;
    _current = {};
    _pending = {};
    TcpMgr::GetInstance()->CloseConnection();
}

// TCP 连上之后发 ID_CHAT_LOGIN 帧，把 uid 与 token 交给聊天服务器
void SessionService::onTcpConnected(bool ok)
{
    if (!_connecting)
        return;
    if (!ok) {
        failAndReset(QStringLiteral("网络异常"));
        return;
    }

    // 长连接已建立，发送聊天服务器登录帧
    QJsonObject frame;
    frame["uid"] = _pending.Uid;
    frame["token"] = _pending.Token;

    const QByteArray payload =
        QJsonDocument(frame).toJson(QJsonDocument::Indented);

    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, payload);
}

// 聊天服务器拒绝登录（token 过期 / 重复登录等）
void SessionService::onChatLoginFailed(int code)
{
    if (!_connecting)
        return;
    failAndReset(QStringLiteral("登录失败，错误码 %1").arg(code));
}

// 聊天服务器登录成功：会话就绪，开始定时心跳
void SessionService::onChatLoggedIn(const QJsonObject &data)
{
    if (!_connecting)
        return;
    if (data.value("uid").toInt() != _pending.Uid) {
        failAndReset(QStringLiteral("聊天服务返回的账号不匹配"));
        return;
    }
    auto *mgr = UserMgr::GetInstance().get();
    mgr->SetUserInfo(std::make_shared<UserInfo>(data["uid"].toInt(), data["name"].toString(),
        data["nick"].toString(), data["icon"].toString(), data["sex"].toInt()));
    mgr->AppendApplyList(data["apply_list"].toArray());
    mgr->AppendFriendList(data["friend_list"].toArray());
    mgr->SetChatThreads(data["thread_list"].toArray());
    for (const auto &value : data["thread_list"].toArray()) {
        const auto item = value.toObject();
        QJsonObject request{{"fromuid", mgr->GetUid()}, {"thread_id", item.value("thread_id")},
                            {"message_id", 0}};
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ,
            QJsonDocument(request).toJson(QJsonDocument::Compact));
    }
    if (data.value("thread_more").toBool()) {
        const auto threads = data.value("thread_list").toArray();
        if (!threads.isEmpty()) {
            QJsonObject request{{"fromuid", mgr->GetUid()},
                {"thread_id", threads.last().toObject().value("thread_id")}};
            emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ,
                QJsonDocument(request).toJson(QJsonDocument::Compact));
        }
    }
    _connecting = false;
    _timeout.stop();
    _ready = true;
    _current = _pending;
    _heartbeat.start();
    emit sessionReady();
}

// 统一失败出口：停表、复位状态，把原因抛给界面
void SessionService::failAndReset(const QString &msg)
{
    closeSession();
    emit connectFailed(msg);
}
