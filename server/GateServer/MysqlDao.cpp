#include "MysqlDao.h"
#include "ConfigMgr.h"

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
