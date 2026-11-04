#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <cstdlib>
#include <algorithm>

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	const auto& host = cfg["Mysql"]["Host"];
	const auto& xport = cfg["Mysql"]["XPort"]; //X Protocol端口号
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	//X Protocol默认端口为33060, 若配置缺失则使用默认值
	unsigned xport_num = 33060;
	if (!xport.empty()) {
		int v = std::atoi(xport.c_str());
		if (v > 0) {
			xport_num = static_cast<unsigned>(v);
		}
	}
	//初始化MySQL连接池对象, 连接池大小为5
	pool_.reset(new MySqlPool(host, xport_num, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close(); //关闭连接池, 释放资源
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回-1
		if (con == nullptr)
			return -1;

		//调用存储过程, 通过会话变量@result返回结果(与JDBC API时代逻辑一致)
		con->_session->sql("CALL reg_user(?,?,?,@result)")
			.bind(name).bind(email).bind(pwd).execute();

		//查询会话变量获取输出参数的值(X DevAPI不直接支持注册输出参数)
		auto res = con->_session->sql("SELECT @result AS result").execute();

		//解析结果, 如果查询成功且有结果, 则获取result值并返回, 否则返回-1表示失败
		auto row = res.fetchOne();
		if (!row.isNull()) {
			int result = row[0].get<int>();
			std::cout << "Result: " << result << std::endl;
			pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
			return result;
		}
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		return -1;
	}catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//执行查询语句(查询指定用户名的邮箱地址)
		auto res = con->_session->sql("SELECT email FROM user WHERE name = ?")
			.bind(name).execute();

		auto row = res.fetchOne();
		//遍历结果集
		while (!row.isNull()) {
			auto email_in_db = row[0].get<std::string>();
			std::cout << "Check Email: " << email_in_db << std::endl;
			//如果查询结果中的email与输入的email不匹配, 则说明用户名和邮箱不匹配, 返回false
			if (email != email_in_db) {
				pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
				return false;
			}
			pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
			return true;
			row = res.fetchOne();
		}
		//保持原行为: 用户名不存在时同样返回true
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//执行更新语句(更新指定用户名的密码)
		auto res = con->_session->sql("UPDATE user SET pwd = ? WHERE name = ?")
			.bind(newpwd).bind(name).execute();

		std::cout << "Updated rows: " << res.getAffectedItemsCount() << std::endl;
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	
	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL会话对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
		});

	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//执行查询语句(查询指定邮箱的用户信息, 显式列出列以便按索引取值)
		auto res = con->_session->sql("SELECT uid, name, pwd, email FROM user WHERE email = ?")
			.bind(email).execute();

		std::string origin_pwd = "";
		auto row = res.fetchOne();
		//遍历结果集
		if (!row.isNull()) {
			origin_pwd = row[2].get<std::string>(); //第2列为pwd
		}

		//如果查询结果中的密码与输入的密码不匹配, 则说明邮箱和密码不匹配, 返回false
		if (pwd != origin_pwd)
			return false;

		//如果邮箱和密码匹配, 则将查询结果中的用户信息存储在userInfo中, 返回true
		userInfo.name = row[1].get<std::string>();
		userInfo.email = email;
		userInfo.uid = row[0].get<int>();
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::AddFriendApply(const int& from, const int& to, const std::string& desc,
	const std::string& back_name)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (con == nullptr)
		return false;

	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL会话对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//申请留言和申请方给对方的备注都保存在申请记录里，同意时直接读取，不另建表
		con->_session->sql(
			"INSERT INTO friend_apply (from_uid, to_uid, descs, back_name) VALUES (?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE descs = VALUES(descs), back_name = VALUES(back_name)")
			.bind(from).bind(to).bind(desc).bind(back_name).execute();

		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name,
	std::string& apply_desc) {
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//开始事务
		con->_session->startTransaction();

        //from是当前同意申请的用户，to是原申请人，所以查找申请时要按to到from的方向匹配
        //FOR UPDATE在事务内锁住这条申请，后面的状态修改和双向好友写入与它一起提交或回滚
        auto apply = con->_session->sql(
            "SELECT status, descs, back_name FROM friend_apply "
            "WHERE from_uid = ? AND to_uid = ? FOR UPDATE")
            .bind(to).bind(from).execute().fetchOne();
        if (apply.isNull()) { //没有对应申请时不允许直接建立好友关系
            con->_session->rollback();
            return false;
        }
        apply_desc = apply[1].get<std::string>();
        const std::string reverse_back = apply[2].get<std::string>();
        //申请已确认时再次设为1不会产生额外状态变化，后续INSERT IGNORE也可避免重复插入好友关系
        con->_session->sql("UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?")
            .bind(to).bind(from).execute();

		//执行第一个插入语句, 插入认证方好友数据
		con->_session->sql("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) ")
			.bind(from).bind(to).bind(back_name).execute();

		//执行第二个插入语句, 插入申请方好友数据(反过来的申请时from, 验证时to)
		con->_session->sql("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) ")
			.bind(to).bind(from).bind(reverse_back).execute();

		//提交事务
		con->_session->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		//如果发生错误, 回滚事务; 连接已断时回滚本身会抛异常, 不能让它逃出本函数
		if (con) {
			try {
				con->_session->rollback();
			}
			catch (mysqlx::Error& rollback_error) {
				std::cerr << "rollback failed, error is " << rollback_error.what() << std::endl;
			}
		}
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回空指针
	if (con == nullptr) {
		return nullptr;
	}
	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL会话对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//执行查询语句(查询指定用户ID的用户信息, 显式列出列以便按索引取值)
		auto res = con->_session->sql("SELECT uid, name, pwd, email, nick, `desc`, sex, icon "
			"FROM user WHERE uid = ?")
			.bind(uid).execute();

		std::shared_ptr<UserInfo> user_ptr = nullptr;
		auto row = res.fetchOne();
		//遍历结果集
		if (!row.isNull()) {
			//如果查询结果中有数据, 则说明用户ID存在, 将查询结果中的用户信息存储在user_ptr中, 返回user_ptr
			user_ptr.reset(new UserInfo);
			user_ptr->uid = row[0].get<int>();
			user_ptr->email = row[3].get<std::string>();
			user_ptr->name = row[1].get<std::string>();
			user_ptr->pwd = row[2].get<std::string>();
			user_ptr->nick = row[4].get<std::string>();
			user_ptr->desc = row[5].get<std::string>();
			user_ptr->sex = row[6].get<int>();
			user_ptr->icon = row[7].get<std::string>();
		}
		return user_ptr;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return nullptr;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(const std::string& name)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回空指针
	if (con == nullptr) {
		return nullptr;
	}
	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL会话对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//执行查询语句(查询指定用户名的用户信息, 与JDBC API时代一致, 不读取icon字段)
		auto res = con->_session->sql("SELECT uid, name, pwd, email, nick, `desc`, sex "
			"FROM user WHERE name = ?")
			.bind(name).execute();

		std::shared_ptr<UserInfo> user_ptr = nullptr;
		auto row = res.fetchOne();
		//遍历结果集
		if (!row.isNull()) {
			//如果查询结果中有数据, 则说明用户名存在, 将查询结果中的用户信息存储在user_ptr中, 返回user_ptr
			user_ptr.reset(new UserInfo);
			user_ptr->uid = row[0].get<int>();
			user_ptr->email = row[3].get<std::string>();
			user_ptr->name = name;
			user_ptr->pwd = row[2].get<std::string>();
			user_ptr->nick = row[4].get<std::string>();
			user_ptr->desc = row[5].get<std::string>();
			user_ptr->sex = row[6].get<int>();
		}
		return user_ptr;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return nullptr;
	}
}

bool MysqlDao::GetApplyList(int touid,
	std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL会话对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
	if (con == nullptr)
		return false;

	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL会话对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//执行查询语句, 根据起始id和限制条数返回列表
		auto res = con->_session->sql("select apply.from_uid, apply.status, user.name, user.nick, user.sex, apply.descs "
			"from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? ")
			.bind(touid).bind(begin).bind(limit).execute();

		auto row = res.fetchOne();
		//遍历结果集
		while (!row.isNull()) {
			auto name = row[2].get<std::string>();
			auto uid = row[0].get<int>();
			auto status = row[1].get<int>();
			auto nick = row[3].get<std::string>();
			auto sex = row[4].get<int>();
			auto desc = row[5].get<std::string>();
			//将查询结果中的好友申请信息封装成ApplyInfo对象
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, desc, "", nick, sex, status);
			applyList.push_back(apply_ptr); //将查询结果中的好友申请信息存储在applyList中
			row = res.fetchOne();
		}

		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info_list)
{
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//执行查询语句, 获取好友id与备注
		auto res = con->_session->sql("select friend_id, `back` from friend where self_id = ? ")
			.bind(self_id).execute();

		auto row = res.fetchOne();
		//遍历结果集
		while (!row.isNull()) {
			auto friend_id = row[0].get<int>();
			//再一次查询friend_id对应的信息
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				row = res.fetchOne();
				continue;
			}

			//保持原逻辑: 备注字段直接使用好友昵称
			user_info->back = user_info->name;
			user_info_list.push_back(user_info);
			row = res.fetchOne();
		}
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

long long MysqlDao::GetOrCreatePrivateChat(int user1, int user2)
{
	if (user1 <= 0 || user2 <= 0 || user1 == user2)
		return 0;
	const int low = std::min(user1, user2);
	const int high = std::max(user1, user2);

	for (int attempt = 0; attempt < 2; ++attempt) {
		auto con = pool_->getConnection();
		if (con == nullptr)
			return 0;
		try {
			con->_session->startTransaction();
		auto friendship = con->_session->sql(
			"SELECT id FROM friend WHERE self_id = ? AND friend_id = ?")
			.bind(user1).bind(user2).execute().fetchOne();
		if (friendship.isNull()) {
			con->_session->rollback();
			pool_->returnConnection(std::move(con));
			return 0;
		}
			auto existing = con->_session->sql(
				"SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ? FOR UPDATE")
				.bind(low).bind(high).execute().fetchOne();
			if (!existing.isNull()) {
				const long long thread_id = existing[0].get<long long>();
				con->_session->commit();
				pool_->returnConnection(std::move(con));
				return thread_id;
			}

			con->_session->sql("INSERT INTO chat_thread (thread_type) VALUES (1)").execute();
			auto id_row = con->_session->sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
			if (id_row.isNull()) {
				con->_session->rollback();
				pool_->returnConnection(std::move(con));
				return 0;
			}
			const long long thread_id = id_row[0].get<long long>();
			con->_session->sql("INSERT INTO private_chat (thread_id, user1_id, user2_id) VALUES (?, ?, ?)")
				.bind(thread_id).bind(low).bind(high).execute();
			con->_session->commit();
			pool_->returnConnection(std::move(con));
			return thread_id;
		}
		catch (mysqlx::Error& e) {
			if (con) {
				try { con->_session->rollback(); } catch (...) {}
				con->_healthy = false;
			}
			pool_->returnConnection(std::move(con));
			if (attempt == 1)
				std::cerr << "Mysqlx Error creating private chat: " << e.what() << std::endl;
		}
	}
	return 0;
}

bool MysqlDao::GetChatThreads(int uid, long long last_thread_id, int limit,
	std::vector<ChatThreadInfo>& threads, bool& has_more)
{
	threads.clear();
	has_more = false;
	limit = std::clamp(limit, 1, 100);
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;
	Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });
	try {
		auto res = con->_session->sql(
			"SELECT pc.thread_id, CASE WHEN pc.user1_id = ? THEN pc.user2_id ELSE pc.user1_id END, "
			"u.name, u.nick, u.icon, u.sex "
			"FROM private_chat pc JOIN user u ON u.uid = CASE WHEN pc.user1_id = ? THEN pc.user2_id ELSE pc.user1_id END "
			"WHERE (pc.user1_id = ? OR pc.user2_id = ?) AND pc.thread_id > ? "
			"ORDER BY pc.thread_id ASC LIMIT ?")
			.bind(uid).bind(uid).bind(uid).bind(uid).bind(last_thread_id).bind(limit + 1).execute();
		auto row = res.fetchOne();
		while (!row.isNull()) {
			if (static_cast<int>(threads.size()) == limit) {
				has_more = true;
				break;
			}
			ChatThreadInfo info;
			info.thread_id = row[0].get<long long>();
			info.peer_uid = row[1].get<int>();
			info.peer_name = row[2].get<std::string>();
			info.peer_nick = row[3].get<std::string>();
			info.peer_icon = row[4].get<std::string>();
			info.peer_sex = row[5].get<int>();
			threads.push_back(std::move(info));
			row = res.fetchOne();
		}
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false;
		std::cerr << "Mysqlx Error loading chat threads: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::GetChatMessages(int uid, long long thread_id, long long last_message_id, int limit,
	std::vector<ChatMessageInfo>& messages, bool& has_more)
{
	messages.clear();
	has_more = false;
	limit = std::clamp(limit, 1, 100);
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;
	Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });
	try {
		auto member = con->_session->sql(
			"SELECT thread_id FROM private_chat WHERE thread_id = ? AND (user1_id = ? OR user2_id = ?)")
			.bind(thread_id).bind(uid).bind(uid).execute().fetchOne();
		if (member.isNull())
			return false;
		auto res = con->_session->sql(
			"SELECT message_id, thread_id, sender_id, recv_id, client_msg_id, content, status "
			"FROM chat_message WHERE thread_id = ? AND message_id > ? "
			"ORDER BY message_id ASC LIMIT ?")
			.bind(thread_id).bind(last_message_id).bind(limit + 1).execute();
		auto row = res.fetchOne();
		while (!row.isNull()) {
			if (static_cast<int>(messages.size()) == limit) {
				has_more = true;
				break;
			}
			ChatMessageInfo info;
			info.message_id = row[0].get<long long>();
			info.thread_id = row[1].get<long long>();
			info.sender_id = row[2].get<int>();
			info.recv_id = row[3].get<int>();
			info.client_msg_id = row[4].get<std::string>();
			info.content = row[5].get<std::string>();
			info.status = row[6].get<int>();
			messages.push_back(std::move(info));
			row = res.fetchOne();
		}
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false;
		std::cerr << "Mysqlx Error loading chat messages: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::SaveChatMessages(int sender_id, int recv_id, long long thread_id,
	const std::vector<std::pair<std::string, std::string>>& contents,
	std::vector<ChatMessageInfo>& messages)
{
	messages.clear();
	if (contents.empty())
		return true;
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;
	if (thread_id <= 0) {
		try {
			auto friendship = con->_session->sql(
				"SELECT id FROM friend WHERE self_id = ? AND friend_id = ?")
				.bind(sender_id).bind(recv_id).execute().fetchOne();
			if (friendship.isNull()) {
				pool_->returnConnection(std::move(con));
				return false;
			}
		} catch (mysqlx::Error& e) {
			con->_healthy = false;
			pool_->returnConnection(std::move(con));
			std::cerr << "Mysqlx Error checking chat friendship: " << e.what() << std::endl;
			return false;
		}
		pool_->returnConnection(std::move(con));
		thread_id = GetOrCreatePrivateChat(sender_id, recv_id);
		con = pool_->getConnection();
		if (con == nullptr)
			return false;
	}
	if (thread_id <= 0)
	{
		pool_->returnConnection(std::move(con));
		return false;
	}
	Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });
	try {
		auto member = con->_session->sql(
			"SELECT thread_id FROM private_chat WHERE thread_id = ? AND ((user1_id = ? AND user2_id = ?) "
			"OR (user1_id = ? AND user2_id = ?))")
			.bind(thread_id).bind(sender_id).bind(recv_id).bind(recv_id).bind(sender_id).execute().fetchOne();
		if (member.isNull())
			return false;

		con->_session->startTransaction();
		for (const auto& [client_id, content] : contents) {
			con->_session->sql(
				"INSERT INTO chat_message (thread_id, sender_id, recv_id, client_msg_id, content) "
				"VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE message_id = LAST_INSERT_ID(message_id)")
				.bind(thread_id).bind(sender_id).bind(recv_id).bind(client_id).bind(content).execute();
			auto id_row = con->_session->sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
			if (id_row.isNull()) {
				con->_session->rollback();
				return false;
			}
			ChatMessageInfo info;
			info.message_id = id_row[0].get<long long>();
			info.thread_id = thread_id;
			info.sender_id = sender_id;
			info.recv_id = recv_id;
			info.client_msg_id = client_id;
			info.content = content;
			messages.push_back(std::move(info));
		}
		con->_session->sql("UPDATE chat_thread SET updated_at = CURRENT_TIMESTAMP WHERE thread_id = ?")
			.bind(thread_id).execute();
		con->_session->commit();
		return true;
	}
	catch (mysqlx::Error& e) {
		if (con) {
			try { con->_session->rollback(); } catch (...) {}
			con->_healthy = false;
		}
		std::cerr << "Mysqlx Error saving chat messages: " << e.what() << std::endl;
		return false;
	}
}


MySqlPool::MySqlPool(const std::string& host, unsigned port, const std::string& user, const std::string& pass,
	const std::string& schema, int poolSize)
    : host_(host), port_(port), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false)
{
	//逐条建立连接, 单条失败不影响其余连接; 数量缺口由检测线程首轮补足
	for (int i = 0; i < poolSize_; ++i) {
		try {
			mysqlx::SessionSettings settings(host_, port_, user_, pass_, schema_);
			auto session = std::make_unique<mysqlx::Session>(settings);
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			pool_.push(std::make_unique<SqlConnection>(std::move(session), timestamp));
			++connection_count_;
		}
		catch (mysqlx::Error& e) {
			std::cout << "mysql connection init failed: " << e.what() << std::endl;
		}
	}

    //启动检测线程, 定期检查连接的状态并保持连接的活跃性(心跳机制)
	_check_thread = std::thread([this]() {
		while (!b_stop_) {
			//锁只覆盖等待本身, 检查与重连全部在锁外执行
			{
				std::unique_lock<std::mutex> lock(mutex_);
				//两个唤醒来源: 停机, 或业务线程丢弃连接后请求补池; 谓词在锁内求值, 不会漏掉通知
				check_cond_.wait_for(lock, std::chrono::seconds(60), [this] {
					return b_stop_.load() || check_requested_;
				});
				if (b_stop_) break;
				check_requested_ = false; //在锁内清除, 与置位方互斥, 本轮未处理的缺口仍会在本轮补足
			}
			checkConnection(); //检查连接的状态并保持连接的活跃性(心跳机制)
		}
	});
}

void MySqlPool::checkConnection() {
    //获取当前连接池中连接的数量, 以便在遍历连接池时不受其他线程影响
    size_t targetCount;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        targetCount = pool_.size();
    }

    size_t processed = 0; //当前已经处理的数量

    //获取当前时间戳
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    while (processed < targetCount) {
        std::unique_ptr<SqlConnection> con; //获取连接对象
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (pool_.empty())
                break;

            con = std::move(pool_.front());
            pool_.pop();
        }

        bool healthy = true;
        //解锁后做检查/重连逻辑
        if (timestamp - con->_last_oper_time >= 5) {
            try {
                con->_session->sql("SELECT 1").execute(); //执行简单查询以保持连接活跃
                con->_last_oper_time = timestamp; //更新最后操作时间
            }
            catch (mysqlx::Error& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                healthy = false;
            }
        }

        //如果连接健康, 则将其归还给连接池; 否则让局部对象析构关闭它, 由本函数末尾的重连补足
        if (healthy)
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (b_stop_) {
                --connection_count_;
            } else {
                pool_.push(std::move(con));
                cond_.notify_one();
            }
        } else {
            std::lock_guard<std::mutex> guard(mutex_);
            --connection_count_;
        }
        ++processed; //更新已处理的数量
    }

    while (!b_stop_) {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            //补足到池容量: 覆盖检查失败、业务线程丢弃、启动时建连失败三种缺口
            if (connection_count_ >= static_cast<size_t>(poolSize_)) break;
        }
        if (!reconnect(timestamp))
            break; //数据库不可达时不在锁外空转, 留到下一个周期再试
    }
}

bool MySqlPool::reconnect(long long timestamp) {
    try {
        //重新创建会话对象并替换旧的会话
        mysqlx::SessionSettings settings(host_, port_, user_, pass_, schema_);
        //X Protocol 下 caching_sha2_password 快速认证需要 TLS, 使用默认 SSL_MODE=REQUIRED(本地开发可接受)
        auto session = std::make_unique<mysqlx::Session>(settings); //替换旧的会话对象

        auto newCon = std::make_unique<SqlConnection>(std::move(session), timestamp);
        {
            std::lock_guard<std::mutex> guard(mutex_);
            //如果连接池被标记为停止, 则直接返回false, 不将新连接对象加入连接池
            if (b_stop_)
                return false;
            pool_.push(std::move(newCon)); //将新连接对象加入连接池队列, 使其可供其他线程使用
            ++connection_count_; //新增连接是池容量增长的唯一来源, 必须与队列同步记账
        }
        cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
        std::cout << "mysql connection reconnect success" << std::endl;
        return true;
    }
    catch (mysqlx::Error& e) {
        std::cout << "Reconnect failed, error is " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//使用条件变量等待连接池中有可用的连接或者连接池被标记为停止
	cond_.wait(lock, [this] {
		//如果连接池被标记为停止, 则直接返回true, 保持锁定状态并继续执行
		if (b_stop_) {
			return true;
		}
		//如果没有可用的连接, 返回false, 解锁并进入等待状态;
		//如果有可用的连接, 返回true, 保持锁定状态并继续执行
		return !pool_.empty(); });
	//如果连接池被标记为停止, 直接返回空指针
	if (b_stop_) {
		return nullptr;
	}
	//从连接池中获取一个可用的连接对象
	std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
	pool_.pop(); //将连接对象从连接池队列中移除
	return con;
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con)
{
	if (!con) return;
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不将连接对象归还给连接池
	if (b_stop_)
		return;
	if (!con->_healthy || !con->_session) {
		//不可复用的连接不回到队列: 先按"空闲+借出"的总数销账, 再请求检测线程补建
		--connection_count_;
		check_requested_ = true;
		lock.unlock(); //关闭会话可能涉及网络收尾, 放到锁外执行
		check_cond_.notify_one();
		return;
	}

	//归还时刷新活跃时间, 检测线程据此跳过刚用过的连接, 只对真正空闲的连接做心跳
	auto now = std::chrono::system_clock::now().time_since_epoch();
	con->_last_oper_time = std::chrono::duration_cast<std::chrono::seconds>(now).count();
	pool_.push(std::move(con)); //将连接对象加入连接池队列, 使其可供其他线程使用
	cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
}

void MySqlPool::Close()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	}
	cond_.notify_all(); //唤醒等待连接的业务线程
	check_cond_.notify_all(); //唤醒周期检测线程
	//等待检测线程退出, 确保后续清理不会与检测线程并发访问连接池
	if (_check_thread.joinable())
		_check_thread.join();

	{
		std::lock_guard<std::mutex> lock(mutex_);
		//检测线程已退出, 队列中的连接不会再被取用, 直接随容器析构关闭
		while (!pool_.empty())
			pool_.pop();
		connection_count_ = 0; //停止后容量计数不再有意义, 清零避免被误读
	}
}

MySqlPool::~MySqlPool()
{
	Close(); //幂等: 调用方已关闭时这里只做一次空转, 保证检测线程一定被回收
}
