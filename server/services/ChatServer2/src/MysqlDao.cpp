#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <cstdlib>

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
		return true;
	}
	catch (mysqlx::Error& e) {
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
			std::cout << "Password: " << origin_pwd << std::endl;
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
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::AddFriendApply(const int& from, const int& to)
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
		//执行插入语句(插入一条好友申请记录, 如果已经存在相同的记录则保持不变)
		con->_session->sql("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid")
			.bind(from).bind(to).execute();

		return true;
	}
	catch (mysqlx::Error& e) {
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to) {
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//执行更新语句, 注意from/to与申请时相反(与JDBC API时代逻辑一致)
		con->_session->sql("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?")
			.bind(to).bind(from).execute();

		return true;
	}
	catch (mysqlx::Error& e) {
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name) {
	auto con = pool_->getConnection();
	if (con == nullptr)
		return false;

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//开始事务
		con->_session->startTransaction();

		//执行第一个插入语句, 插入认证方好友数据
		con->_session->sql("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) ")
			.bind(from).bind(to).bind(back_name).execute();

		//执行第二个插入语句, 插入申请方好友数据(反过来的申请时from, 验证时to)
		con->_session->sql("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) ")
			.bind(to).bind(from).bind("").execute();

		//提交事务
		con->_session->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (mysqlx::Error& e) {
		//如果发生错误, 回滚事务
		if (con) {
			con->_session->rollback();
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
		auto res = con->_session->sql("select apply.from_uid, apply.status, user.name, user.nick, user.sex "
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
			//将查询结果中的好友申请信息封装成ApplyInfo对象
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			applyList.push_back(apply_ptr); //将查询结果中的好友申请信息存储在applyList中
			row = res.fetchOne();
		}
		return true;
	}
	catch (mysqlx::Error& e) {
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
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
		return false;
	}
}


MySqlPool::MySqlPool(const std::string& host, unsigned port, const std::string& user, const std::string& pass,
	const std::string& schema, int poolSize)
		: host_(host), port_(port), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false)
{
	try {
		for (int i = 0; i < poolSize_; ++i) {
			//构建X DevAPI连接设置
			mysqlx::SessionSettings settings(host_, port_, user_, pass_, schema_);
			// X Protocol 下 caching_sha2_password 快速认证需要 TLS, 使用默认 SSL_MODE=REQUIRED(本地开发可接受)
			//创建会话对象
			auto session = std::make_unique<mysqlx::Session>(settings);
			//获取当前时间戳
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			//将时间戳转换为秒
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			//将会话对象和当前时间封装成SqlConnection对象, 加入连接池队列
			pool_.push(std::make_unique<SqlConnection>(std::move(session), timestamp));
		}

		//启动检测线程, 定期检查连接的状态并保持连接的活跃性, 防止连接超时导致连接池中的连接失效
		_check_thread = std::thread([this]() {
			//如果不是停止状态
			while (!b_stop_) {
				checkConnection(); //检查连接的状态
				std::this_thread::sleep_for(std::chrono::seconds(60)); //每60秒检查一次
			}
		});
		_check_thread.detach(); //分离检测线程, 使其在后台运行, 不阻塞主线程的退出
	}
	catch (mysqlx::Error& e) {
		//处理异常
		std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
	}
}

void MySqlPool::checkConnection() {
	std::lock_guard<std::mutex> guard(mutex_); //加锁保护连接池的状态
	int poolsize = pool_.size(); //获取当前连接池中的连接数量
	
	auto currentTime = std::chrono::system_clock::now().time_since_epoch(); //获取当前时间戳
	//将时间戳转换为秒
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

	for (int i = 0; i < poolsize; i++) {
		//从连接池队列中取出一个连接对象进行检查
		auto con = std::move(pool_.front());
		pool_.pop();

		//使用Defer对象确保检查完成后将连接对象归还给连接池, 无论检查过程中是否发生异常
		Defer defer([this, &con]() {
			pool_.push(std::move(con));
			});

		//如果当前时间与连接对象的上次操作时间的差值小于30秒, 则说明连接仍然活跃, 不需要检查, 直接跳过
		if (timestamp - con->_last_oper_time < 30)
			continue;

		try {
			//执行一个简单的查询语句, 保证连接的活跃性
			con->_session->sql("SELECT 1").execute();
			con->_last_oper_time = timestamp; //更新连接对象的上次操作时间为当前时间
		}
		catch (mysqlx::Error& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			//重新创建会话对象并替换旧的会话
			mysqlx::SessionSettings settings(host_, port_, user_, pass_, schema_);
			// X Protocol 下 caching_sha2_password 快速认证需要 TLS, 使用默认 SSL_MODE=REQUIRED(本地开发可接受)
			con->_session.reset(new mysqlx::Session(settings)); //替换旧的会话对象
			con->_last_oper_time = timestamp; //更新连接对象的上次操作时间为当前时间
		}
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
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//如果连接池被标记为停止, 则直接返回, 不将连接对象归还给连接池
	if (b_stop_) {
		return;
	}

	pool_.push(std::move(con)); //将连接对象加入连接池队列, 使其可供其他线程使用
	cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
}

void MySqlPool::Close()
{
	b_stop_ = true; //设置停止标志, 使连接池进入停止状态
	cond_.notify_all(); //通知所有等待连接的线程, 使它们能够退出等待状态并检查停止标志
}

MySqlPool::~MySqlPool()
{
	std::unique_lock<std::mutex> lock(mutex_); //加锁保护连接池的状态
	//清空连接池队列
	while (!pool_.empty()) {
		pool_.pop();
	}
}
