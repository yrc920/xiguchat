#ifndef CHATBRIDGE_H
#define CHATBRIDGE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "friendlistmodel.h"
#include "applylistmodel.h"
#include "messagemodel.h"

/**
 * @brief 聊天主界面的桥接对象
 *
 * 界面左边显示什么、右边显示什么，全部由这里的 mode / pane 决定：
 * 侧栏点「聊天」或「联系人」会同时改这两项，列表点条目则改对应的一项。
 * QML 只做渲染，自己不保存导航状态。
 *
 *   点侧栏「聊天」   → mode=会话，右侧回到当前会话（没有会话则为空）
 *   点侧栏「联系人」 → mode=联系人，右侧显示「新的朋友」
 *   会话条目         → 右侧聊天页
 *   联系人条目       → 右侧好友资料页
 *   资料页「发消息」 → 回到会话页
 *
 * 同时托管：三个列表模型（会话与联系人共用一个好友模型，申请单独一个）、
 * 搜索状态机（搜到自己忽略、已是好友直接跳会话、陌生人交界面弹窗），
 * 以及发消息、加好友、同意申请的协议调用。
 */
class ChatBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(FriendListModel *friends READ friends CONSTANT)
    Q_PROPERTY(ApplyListModel *applies READ applies CONSTANT)
    Q_PROPERTY(MessageModel *messages READ messages CONSTANT)
    Q_PROPERTY(int mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(int pane READ pane NOTIFY paneChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(int currentUid READ currentUid NOTIFY currentChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentChanged)
    Q_PROPERTY(QString myIcon READ myIcon CONSTANT)
    Q_PROPERTY(QString myName READ myName CONSTANT)
    Q_PROPERTY(QString myUserName READ myUserName CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int searchState READ searchState NOTIFY searchStateChanged)
    Q_PROPERTY(QVariantMap searchResult READ searchResult NOTIFY searchResultChanged)
    Q_PROPERTY(QString searchMessage READ searchMessage NOTIFY searchStateChanged)

public:
    // 左侧列表：会话 / 联系人
    enum Mode { ChatMode = 0, ContactMode = 1 };
    // 右侧页面：空 / 聊天 / 好友资料 / 好友申请，取值与 QML 里的 pane 判断一致
    enum Pane { EmptyPane = 0, ChatPage = 1, FriendInfoPage = 2, ApplyFriendPage = 3 };
    Q_ENUM(Mode)
    Q_ENUM(Pane)

    explicit ChatBridge(QObject *parent = nullptr);

    FriendListModel *friends() const { return _friends; }
    ApplyListModel *applies() const { return _applies; }
    MessageModel *messages() const { return _messages; }

    int mode() const { return _mode; }
    int pane() const { return _pane; }
    QVariantMap profile() const { return _profile; }

    // 当前会话
    int currentUid() const { return _currentUid; }
    QString currentName() const { return _currentName; }

    // 自己：myName 用于显示（昵称优先），myUserName 用于申请文案（用户名）
    QString myIcon() const;
    QString myName() const;
    QString myUserName() const;

    QString searchText() const { return _searchText; }
    void setSearchText(const QString &text);

    // 搜索状态：0 空闲 / 1 搜索中 / 2 找到陌生人 / 3 没找到
    int searchState() const { return _searchState; }
    QVariantMap searchResult() const { return _searchResult; }
    QString searchMessage() const { return _searchMessage; }

    // 侧栏两个按钮
    Q_INVOKABLE void showChatMode();
    Q_INVOKABLE void showContactMode();

    // 右侧三页的入口
    Q_INVOKABLE void openChat(int uid);
    Q_INVOKABLE void openProfile(int uid);
    Q_INVOKABLE void openApplyList();

    // 业务动作
    Q_INVOKABLE bool sendParts(const QVariantList &parts);
    Q_INVOKABLE void search();
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE void applyTo(int uid, const QString &verify, const QString &remark);
    Q_INVOKABLE void authApply(int uid, const QString &remark);

signals:
    void modeChanged();
    void paneChanged();
    void profileChanged();
    void currentChanged();
    void searchTextChanged();
    void searchStateChanged();
    void searchResultChanged();

private:
    void setMode(int mode);
    void setPane(int pane);

    FriendListModel *_friends = nullptr;
    ApplyListModel *_applies = nullptr;
    MessageModel *_messages = nullptr;

    int _mode = ChatMode;
    int _pane = EmptyPane;
    QVariantMap _profile; // 资料页展示的好友信息

    int _currentUid = 0;
    QString _currentName;
    QString _searchText;
    int _searchState = 0;
    QVariantMap _searchResult;
    QString _searchMessage;
};

#endif // CHATBRIDGE_H
