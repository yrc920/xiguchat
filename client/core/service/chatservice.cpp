#include "chatservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "sessionservice.h"
#include "tcpmgr.h"
#include "usermgr.h"

void ChatService::ensurePrivateChat(int uid)
{
    auto *mgr = UserMgr::GetInstance().get();
    if (!SessionService::GetInstance()->isReady() || uid <= 0 || mgr->ThreadId(uid) > 0
        || mgr->HasPendingPrivateChat(uid))
        return;
    mgr->MarkPrivateChatPending(uid, true);
    QJsonObject request{{"fromuid", mgr->GetUid()}, {"touid", uid}};
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CREATE_PRIVATE_CHAT_REQ,
        QJsonDocument(request).toJson(QJsonDocument::Compact));
}

bool ChatService::sendText(int uid, const QString &text)
{
    auto *mgr = UserMgr::GetInstance().get();
    if (!SessionService::GetInstance()->isReady() || !mgr->GetFriendById(uid))
        return false;
    ensurePrivateChat(uid);
    if (text.isEmpty())
        return true;

    const int from = mgr->GetUid();
    qsizetype offset = 0;
    while (offset < text.size()) {
        const QString msgId = QUuid::createUuid().toString();
        QJsonObject item{{"msgid", msgId}};
        QJsonObject frame{{"fromuid", from}, {"touid", uid}};
        frame["thread_id"] = mgr->ThreadId(uid);
        QByteArray payload;
        QString content;

        // 服务端接收上限为 2048 字节。按 JSON 编码后的字节数分帧，
        // 预留转发时的 error 字段与格式化开销；UTF-16 代理对不可拆开。
        while (offset < text.size()) {
            const qsizetype count = text.at(offset).isHighSurrogate()
                && offset + 1 < text.size() && text.at(offset + 1).isLowSurrogate() ? 2 : 1;
            const QString next = content + text.mid(offset, count);
            item["content"] = next;
            frame["text_array"] = QJsonArray{item};
            const QByteArray encoded = QJsonDocument(frame).toJson(QJsonDocument::Compact);
            if (encoded.size() > 1800)
                break;
            content = next;
            payload = encoded;
            offset += count;
        }

        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, payload);
        mgr->AppendFriendChatMsg(uid, {
            std::make_shared<TextChatData>(msgId, content, from, uid, 0, mgr->ThreadId(uid), ChatMessageStatus::Sending)
        });
    }
    return true;
}
