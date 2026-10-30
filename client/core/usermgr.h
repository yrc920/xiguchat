#ifndef USERMGR_H
#define USERMGR_H

#include <QObject>
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

    void AppendApplyList(QJsonArray array);
    void AppendFriendList(QJsonArray array);
    bool AlreadyApply(int uid); //检查是否已经有来自指定用户ID的好友申请
    void AddApplyList(std::shared_ptr<ApplyInfo> app); //添加好友申请到申请列表
    void SetApplyStatus(int uid, int status); //更新某条申请的处理状态（0 待处理 / 1 已添加）
    bool CheckFriendById(int uid); //检查指定用户ID是否是好友
    void AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void AddFriend(std::shared_ptr<AuthInfo> auth_info);
    void AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs);

    // 供列表模型按行读取：模型不复制数据，只引用这里的原始对象
    int FriendCount() const;
    std::shared_ptr<FriendInfo> FriendAt(int index) const;
    std::shared_ptr<FriendInfo> GetFriendById(int uid);
    int ApplyCount() const;
    std::shared_ptr<ApplyInfo> ApplyAt(int index) const;

    // 新会话开始前清空上一个账号的数据，避免重新登录后好友与申请重复
    void ClearSession();

signals:
    void FriendListAboutToChange();
    void ApplyListAboutToChange();
    void FriendListChanged();        // 登录拿到好友列表、通过好友申请
    void ApplyListChanged();         // 好友申请列表变化
    void FriendChatChanged(int uid); // 某个好友的消息变化

private:
    UserMgr();

    std::shared_ptr<UserInfo> _user_info;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list; //好友申请列表
    std::vector<std::shared_ptr<FriendInfo>> _friend_list;
    QMap<int, std::shared_ptr<FriendInfo>> _friend_map;
};

#endif // USERMGR_H
