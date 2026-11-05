#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
#include <QByteArray>
#include <memory>
#include <singleton.h>
#include "userdata.h"
#include <vector>
#include <QMap>
/******************************************************************************
 *
 * @file       usermgr.h
 * @brief      用户管理类，负责保存用户信息
 *
 * @author     西故
 * @date       2026/05/03
 * @history
 *****************************************************************************/

class UserMgr : public QObject,public Singleton<UserMgr>,
    public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT //使得UserMgr可以使用Qt的信号和槽机制

public:
    friend class Singleton<UserMgr>; //使基类创建实例时可以访问私有构造函数
    ~UserMgr(); //使基类析构时可以访问派生类的析构函数

    void SetUserInfo(std::shared_ptr<UserInfo> user_info); //设置用户信息
    std::shared_ptr<UserInfo> GetUserInfo();
    int GetUid(); //获取用户ID
    static QString LocalAvatarPath(); //当前网关与账号对应的头像文件
    QString GetIcon() const;
    bool SetLocalAvatar(const QByteArray &png); //发布已落盘的头像，不修改服务器头像字段

    void AppendApplyList(QJsonArray array);
    void AppendFriendList(QJsonArray array);
    bool AlreadyApply(int uid); //检查是否已经有来自指定用户ID的好友申请
    void AddApplyList(std::shared_ptr<ApplyInfo> app); //添加好友申请到申请列表
    void SetApplyStatus(int uid, int status); //更新某条申请的处理状态（0 待处理 / 1 已添加）
    bool CheckFriendById(int uid); //检查指定用户ID是否是好友
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);
    void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs); //按 msgid 去重后并入会话
    void SetChatThreads(const QJsonArray &threads); //登录时重置并写入全部会话映射
    void AppendChatThreads(const QJsonArray &threads); //追加会话映射, 不动已有消息
    void RefreshFriendList(); //仅通知列表重建, 数据已由调用方改好
    void SetThreadId(int friend_id, qint64 thread_id); //补会话ID并清除 pending
    qint64 ThreadId(int friend_id) const; //好友对应的会话ID, 0 表示还没有
    int FriendIdByThread(qint64 thread_id) const; //反查会话属于哪个好友
    qint64 LastMessageId(int friend_id) const; //当前最大消息ID, 作为翻页游标
    bool HasPendingPrivateChat(int friend_id) const; //是否正在等待 1026 回包
    void MarkPrivateChatPending(int friend_id, bool pending);
    void UpdateChatMessage(int friend_id, const QString &msg_id, qint64 message_id,
                           ChatMessageStatus status); //回执补 ID/状态并重新排序
    void FailPendingMessages(); //断线时把发送中的消息统一标失败

    // 供列表模型按行读取：模型不复制数据，只引用这里的原始对象
    int FriendCount() const;
    std::shared_ptr<FriendInfo> FriendAt(int index) const;
    std::shared_ptr<FriendInfo> GetFriendById(int uid);
    int ApplyCount() const;
    std::shared_ptr<ApplyInfo> ApplyAt(int index) const;

    // 新会话开始前清空上一个账号的数据，避免重新登录后好友与申请重复
    void ClearSession();

signals:
    void UserIconChanged();
    void FriendListAboutToChange();
    void ApplyListAboutToChange();
    void FriendListChanged();        // 登录拿到好友列表、通过好友申请
    void ApplyListChanged();         // 好友申请列表变化
    void FriendChatChanged(int uid); // 某个好友的消息变化

private:
    UserMgr();

    std::shared_ptr<UserInfo> _user_info;
    QString _localIcon; //本机头像 data URL, 为空时回落到服务端 icon
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list; //好友申请列表
    std::vector<std::shared_ptr<FriendInfo>> _friend_list; //列表模型按行读取的顺序容器
    QMap<int, std::shared_ptr<FriendInfo>> _friend_map; //uid 索引, 与列表指向同一对象
    QMap<int, bool> _pending_private_chats; //正在等待建会话回包的好友, 防止重复请求
};

#endif // USERMGR_H
