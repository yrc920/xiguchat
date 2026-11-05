#pragma once
// 用于管理MySQL连接池并提供MySQL操作接口
//

#include "MysqlDao.h"
#include "Singleton.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>; //使基类创建实例时可以访问私有构造函数
public:
	~MysqlMgr(); //使基类析构时可以访问派生类的析构函数

	//注册用户, 返回用户ID, 如果注册失败则返回-1
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email); //检查用户名和邮箱是否匹配
	bool UpdatePwd(const std::string& name, const std::string& pwd); //更新密码
	//检查邮箱和密码是否匹配, 如果匹配则将用户信息存储在userInfo中
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	bool AddFriendApply(const int& from, const int& to, const std::string& desc,
		const std::string& back_name); //添加好友申请、申请留言与备注
	bool AddFriend(const int& from, const int& to, std::string back_name, std::string& apply_desc);

	std::shared_ptr<UserInfo> GetUser(int uid); //根据用户ID获取用户信息
	std::shared_ptr<UserInfo> GetUser(const std::string& name); //根据用户名获取用户信息
	//获取好友申请列表, 根据起始id和限制条数返回列表
	bool GetApplyList(int touid,
		std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit = 10);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
	long long GetOrCreatePrivateChat(int user1, int user2); //取单聊会话, 没有则创建
	//分页读取会话列表与消息, 具体语义见 MysqlDao 中同名方法
	bool GetChatThreads(int uid, long long last_thread_id, int limit,
		std::vector<ChatThreadInfo>& threads, bool& has_more);
	bool GetChatMessages(int uid, long long thread_id, long long last_message_id, int limit,
		std::vector<ChatMessageInfo>& messages, bool& has_more);
	bool SaveChatMessages(int sender_id, int recv_id, long long thread_id,
		const std::vector<std::pair<std::string, std::string>>& contents,
		std::vector<ChatMessageInfo>& messages);

private:
	MysqlMgr();

	MysqlDao _dao; //数据库访问对象
};
