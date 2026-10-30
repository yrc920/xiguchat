#include "chatbridge.h"

#include <QUuid>

#include "chatservice.h"
#include "sessionservice.h"
#include "friendservice.h"
#include "usermgr.h"
#include "userdata.h"

ChatBridge::ChatBridge(QObject *parent) : QObject(parent)
{
    _friends = new FriendListModel(this);
    _applies = new ApplyListModel(this);
    _messages = new MessageModel(this);

    auto *friendService = FriendService::GetInstance().get();
    connect(friendService, &FriendService::userFound, this,
            [this](int uid, const QString &name, const QString &nick,
                   const QString &desc, const QString &icon, int) {
                if (_searchState != 1)
                    return;
                // 搜到自己：没有意义，直接退掉搜索态
                if (uid == UserMgr::GetInstance()->GetUid()) {
                    clearSearch();
                    return;
                }

                // 已经是好友：不弹「添加」，直接跳到这个人的会话
                if (UserMgr::GetInstance()->CheckFriendById(uid)) {
                    clearSearch();
                    openChat(uid);
                    return;
                }

                // 陌生人：把结果交给界面弹确认窗
                _searchResult = {{QStringLiteral("uid"), uid},
                                 {QStringLiteral("name"), name},
                                 {QStringLiteral("nick"), nick},
                                 {QStringLiteral("icon"), icon},
                                 {QStringLiteral("desc"), desc}};
                _searchState = 2;
                _searchMessage.clear();
                emit searchResultChanged();
                emit searchStateChanged();
            });
    connect(friendService, &FriendService::searchFailed, this,
            [this](const QString &message) {
                if (_searchState != 1)
                    return;
                _searchResult.clear();
                _searchState = 3;
                _searchMessage = message;
                emit searchResultChanged();
                emit searchStateChanged();
            });

    // 进入聊天界面时默认打开第一个会话，避免右侧一片空白
    if (_friends->rowCount() > 0) {
        const QModelIndex first = _friends->index(0, 0);
        openChat(first.data(FriendListModel::UidRole).toInt());
    }
}

void ChatBridge::setSearchText(const QString &text)
{
    if (_searchText == text)
        return;

    _searchText = text;
    emit searchTextChanged();

    // 输入框被清空时顺手退出搜索态，弹窗和结果都要收掉
    if (text.trimmed().isEmpty())
        clearSearch();
}

void ChatBridge::search()
{
    const QString text = _searchText.trimmed();
    if (text.isEmpty() || _searchState == 1 || !SessionService::GetInstance()->isReady())
        return;

    // 先切到「搜索中」，界面据此禁用重复回车
    _searchState = 1;
    _searchMessage.clear();
    emit searchStateChanged();

    if (!FriendService::GetInstance()->searchUser(text)) {
        _searchState = 3;
        _searchMessage = tr("上一次搜索尚未结束，请稍后重试");
        emit searchStateChanged();
    }
}

void ChatBridge::clearSearch()
{
    if (_searchState == 0 && _searchResult.isEmpty() && _searchText.isEmpty())
        return;

    _searchText.clear();
    _searchResult.clear();
    _searchMessage.clear();
    _searchState = 0;
    emit searchTextChanged();
    emit searchResultChanged();
    emit searchStateChanged();
}

void ChatBridge::applyTo(int uid, const QString &verify, const QString &remark)
{
    FriendService::GetInstance()->applyFriend(uid, verify, remark);
}

void ChatBridge::authApply(int uid, const QString &remark)
{
    FriendService::GetInstance()->authFriend(uid, remark);
}

QString ChatBridge::myIcon() const
{
    auto info = UserMgr::GetInstance()->GetUserInfo();
    return info ? info->_icon : QString();
}

QString ChatBridge::myName() const
{
    auto info = UserMgr::GetInstance()->GetUserInfo();
    if (!info)
        return {};

    // 昵称优先，没有昵称才退回用户名
    return info->_nick.isEmpty() ? info->_name : info->_nick;
}

QString ChatBridge::myUserName() const
{
    auto info = UserMgr::GetInstance()->GetUserInfo();
    return info ? info->_name : QString();
}

void ChatBridge::showChatMode()
{
    clearSearch();
    setMode(ChatMode);

    // 有当前会话就回到会话页，没有就停在空态
    setPane(_currentUid != 0 ? ChatPage : EmptyPane);
}

void ChatBridge::showContactMode()
{
    clearSearch();
    setMode(ContactMode);

    // 点「联系人」直接落在「新的朋友」，这是列表里第一个入口
    setPane(ApplyFriendPage);
}

void ChatBridge::openChat(int uid)
{
    auto info = UserMgr::GetInstance()->GetFriendById(uid);
    if (!info)
        return;

    clearSearch();

    _currentUid = uid;
    _currentName = info->_nick.isEmpty() ? info->_name : info->_nick;

    // 消息模型切到这个人，随后触发的 countChanged 会让界面滚到底
    _messages->setFriend(uid);
    emit currentChanged();

    setMode(ChatMode);
    setPane(ChatPage);
}

void ChatBridge::openProfile(int uid)
{
    auto info = UserMgr::GetInstance()->GetFriendById(uid);
    if (!info)
        return;

    // 界面只认这一份快照，字段缺失时 QML 侧有兜底
    _profile = {
        {QStringLiteral("uid"), uid},
        {QStringLiteral("name"), info->_name},
        {QStringLiteral("icon"), info->_icon},
        {QStringLiteral("nick"), info->_nick},
        {QStringLiteral("remark"), info->_back},
        {QStringLiteral("desc"), info->_desc},
        {QStringLiteral("sex"), info->_sex},
        {QStringLiteral("initial"), info->_name.isEmpty() ? QStringLiteral("?")
                                                         : info->_name.left(1)},
    };
    emit profileChanged();

    setMode(ContactMode);
    setPane(FriendInfoPage);
}

void ChatBridge::openApplyList()
{
    setMode(ContactMode);
    setPane(ApplyFriendPage);
}

void ChatBridge::setMode(int mode)
{
    if (_mode == mode)
        return;
    _mode = mode;
    emit modeChanged();
}

void ChatBridge::setPane(int pane)
{
    if (_pane == pane)
        return;
    _pane = pane;
    emit paneChanged();
}

bool ChatBridge::sendParts(const QVariantList &parts)
{
    auto *mgr = UserMgr::GetInstance().get();
    if (!SessionService::GetInstance()->isReady() || !mgr->GetFriendById(_currentUid))
        return false;

    bool hasContent = false;
    for (const QVariant &part : parts) {
        const QVariantMap item = part.toMap();
        hasContent |= item.value("kind").toString() == "image"
            || !item.value("text").toString().trimmed().isEmpty();
    }
    if (!hasContent)
        return false;

    for (const QVariant &part : parts) {
        const QVariantMap item = part.toMap();
        if (item.value("kind").toString() == "image") {
            auto message = std::make_shared<TextChatData>(QUuid::createUuid().toString(),
                                                        QString(), mgr->GetUid(), _currentUid);
            message->_imageSource = item.value("source").toString();
            message->_imageWidth = item.value("width").toInt();
            message->_imageHeight = item.value("height").toInt();
            mgr->AppendFriendChatMsg(_currentUid, {message});
        } else if (!ChatService::GetInstance()->sendText(_currentUid, item.value("text").toString())) {
            return false;
        }
    }
    return true;
}
