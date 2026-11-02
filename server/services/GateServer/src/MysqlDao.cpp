#include "MysqlDao.h"
#include "ConfigMgr.h"
#include "PasswordHash.h"
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

		//密码以哈希形式落库, 数据库中不出现明文
		auto stored_pwd = PasswordHash::Hash(pwd);
		if (stored_pwd.empty()) {
			pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
			return -1;
		}

		//调用存储过程, 通过会话变量@result返回结果(与JDBC API时代逻辑一致)
		con->_session->sql("CALL reg_user(?,?,?,@result)")
			.bind(name).bind(email).bind(stored_pwd).execute();

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

		//密码以哈希形式落库, 数据库中不出现明文
		auto stored_pwd = PasswordHash::Hash(newpwd);
		if (stored_pwd.empty()) {
			pool_->returnConnection(std::move(con)); //将使用完的会话对象归还给连接池
			return false;
		}

		//执行更新语句(更新指定用户名的密码)
		auto res = con->_session->sql("UPDATE user SET pwd = ? WHERE name = ?")
			.bind(stored_pwd).bind(name).execute();

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

		auto row = res.fetchOne();
		//未查到该邮箱对应的用户, 按密码不匹配处理
		if (row.isNull())
			return false;

		std::string origin_pwd = row[2].get<std::string>(); //第2列为pwd
		userInfo.name = row[1].get<std::string>();
		userInfo.email = email;
		userInfo.uid = row[0].get<int>();
		userInfo.pwd = origin_pwd;

		//库中 pwd 为 Argon2id 哈希串, 直接校验
		return PasswordHash::Verify(pwd, origin_pwd);
	}
	catch (mysqlx::Error& e) {
		if (con) con->_healthy = false; //标记为不可复用, 归还时丢弃
		std::cerr << "Mysqlx Error: " << e.what() << std::endl;
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
			checkConnection();
		}
	});
}

void MySqlPool::checkConnection() {
	size_t poolsize;
	{
	    std::lock_guard<std::mutex> guard(mutex_);
	    poolsize = pool_.size(); //获取当前连接池中连接的数量, 以便在遍历连接池时不受其他线程影响
	}

    //获取当前时间戳
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

	for (size_t i = 0; i < poolsize; i++) {
		//从连接池队列中取出一个连接对象进行检查
		std::unique_ptr<SqlConnection> con;
		{
		    std::lock_guard<std::mutex> guard(mutex_);
		    //如果连接池为空, 则直接退出循环, 避免访问空队列
		    if (pool_.empty())
		        break;
		    con = std::move(pool_.front());
		    pool_.pop();
		}

		bool healthy = true;
		try {
		    //如果连接对象的最后操作时间距当前时间超过5秒, 则执行简单查询以保持连接活跃
			if (timestamp - con->_last_oper_time >= 5) {
				con->_session->sql("SELECT 1").execute(); //执行简单查询以保持连接活跃
				con->_last_oper_time = timestamp; //更新最后操作时间戳, 以便下次检查时判断是否需要保持连接活跃
			}
		}
		catch (mysqlx::Error& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			healthy = false;
		}
		std::lock_guard<std::mutex> guard(mutex_);
		if (healthy && !b_stop_) {
			pool_.push(std::move(con));
			cond_.notify_one();
		} else {
			--connection_count_;
		}
	}
	while (!b_stop_) {
		{
			std::lock_guard<std::mutex> guard(mutex_);
			//补足到池容量: 覆盖检查失败、业务线程丢弃、启动时建连失败三种缺口
			if (connection_count_ >= static_cast<size_t>(poolSize_)) break;
		}
		if (!reconnect(timestamp)) break; //数据库不可达时不在锁外空转, 留到下一个周期再试
	}
}

bool MySqlPool::reconnect(long long timestamp) {
	try {
	    //构建X DevAPI连接设置
		mysqlx::SessionSettings settings(host_, port_, user_, pass_, schema_);
		auto session = std::make_unique<mysqlx::Session>(settings);
		auto con = std::make_unique<SqlConnection>(std::move(session), timestamp);
		{
		    std::lock_guard<std::mutex> lock(mutex_);
		    if (b_stop_)
		        return false;
		    pool_.push(std::move(con)); //将新连接对象加入连接池队列, 使其可供其他线程使用
			++connection_count_; //新增连接是池容量增长的唯一来源, 必须与队列同步记账
		}
		cond_.notify_one(); //通知一个等待的线程, 使其从连接池中获取一个可用的连接对象
	    return true;
	} catch (mysqlx::Error& e){
	    std::cerr << "MySQL reconnect failed: " << e.what() << std::endl;
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
		return !pool_.empty();
	});
	//如果连接池被标记为停止, 直接返回空指针
	if (b_stop_)
		return nullptr;

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
	    b_stop_ = true; //标记连接池为停止
	}
	cond_.notify_all(); //唤醒等待连接的业务线程
	check_cond_.notify_all(); //唤醒周期检测线程
    //等待检测线程退出, 确保在析构函数中不会有线程访问已释放的资源
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
