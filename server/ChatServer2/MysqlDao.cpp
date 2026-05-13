#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <iostream>
#include <chrono>

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	//初始化MySQL连接池对象, 连接池大小为5
	pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close(); //关闭连接池, 释放资源
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL连接对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->
			prepareStatement("CALL reg_user(?,?,?,@result)"));
		//设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);
		stmt->execute();//执行存储过程

		//查询会话变量获取输出参数的值(PreparedStatement不直接支持注册输出参数)
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		//执行查询获取结果, 此处逻辑写在mysql存储过程中, 通过设置会话变量@result来返回结果
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));

		//解析结果, 如果查询成功且有结果, 则获取result值并返回, 否则返回-1表示失败
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
			return result;
		}
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		return -1;
	}catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL连接对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//准备查询语句(查询指定用户名的邮箱地址)
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->
			prepareStatement("SELECT email FROM user WHERE name = ?"));
		pstmt->setString(1, name); //绑定参数
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery()); //执行查询

		//遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			//如果查询结果中的email与输入的email不匹配, 则说明用户名和邮箱不匹配, 返回false
			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
				return false;
			}
			pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
			return true;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL连接对象
	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//准备查询语句(更新指定用户名的密码)
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->
			prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		//绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);
		int updateCount = pstmt->executeUpdate(); //执行更新

		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL连接对象
	
	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL连接对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con)); //将使用完的MySQL连接对象归还给连接池
		});

	try {
		//如果连接池返回空指针, 则说明连接池已停止, 直接返回false
		if (con == nullptr)
			return false;

		//准备查询语句(查询指定邮箱的用户信息, 包含密码、邮箱等字段)
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->
			prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); //绑定参数
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery()); //执行查询

		std::string origin_pwd = "";
		//遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd"); //获取查询结果中的密码字段值
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		//如果查询结果中的密码与输入的密码不匹配, 则说明邮箱和密码不匹配, 返回false
		if (pwd != origin_pwd)
			return false;

		//如果邮箱和密码匹配, 则将查询结果中的用户信息存储在userInfo中, 返回true
		userInfo.name = res->getString("name");
		userInfo.email = email;
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
	auto con = pool_->getConnection(); //从连接池中获取一个可用的MySQL连接对象
	//如果连接池返回空指针, 则说明连接池已停止, 直接返回空指针
	if (con == nullptr) {
		return nullptr;
	}
	//使用RAII机制确保无论函数如何退出, 都能正确归还MySQL连接对象给连接池, 避免资源泄漏
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//准备SQL语句(查询指定用户ID的用户信息, 包含用户名、邮箱、密码等字段)
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->
			prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid); //绑定参数
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery()); //执行查询

		std::shared_ptr<UserInfo> user_ptr = nullptr;
		//遍历结果集
		while (res->next()) {
			//如果查询结果中有数据, 则说明用户ID存在, 将查询结果中的用户信息存储在user_ptr中, 返回user_ptr
			user_ptr.reset(new UserInfo);
			user_ptr->uid = uid;
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->pwd = res->getString("pwd");
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}


MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, 
	const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false)
{
	try {
		for (int i = 0; i < poolSize_; ++i) {
			//获取MySQL驱动实例
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* con = driver->connect(url_, user_, pass_); //创建MySQL连接对象
			con->setSchema(schema_); //设置连接的默认数据库
			// 获取当前时间戳
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			// 将时间戳转换为秒
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			//将连接对象和当前时间封装成SqlConnection对象, 加入连接池队列
			pool_.push(std::make_unique<SqlConnection>(con, timestamp));
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
	catch (sql::SQLException& e) {
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
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_last_oper_time = timestamp; //更新连接对象的上次操作时间为当前时间
		}
		catch (sql::SQLException& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			//重新创建连接并替换旧的连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance(); //获取MySQL驱动实例
			auto* newcon = driver->connect(url_, user_, pass_); //创建新的MySQL连接对象
			newcon->setSchema(schema_); //设置连接的默认数据库
			con->_con.reset(newcon); //替换旧的连接对象
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
