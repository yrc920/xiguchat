#include "LogicSystem.h"
#include <thread>
#include <mutex>
#include <csignal>
#include "IOContextPool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"

int main()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	auto server_name = cfg["SelfServer"]["Name"]; //从配置文件中获取自身服务器的名称
	try {
		auto pool = IOContextPool::GetInstance(); //获取IOContextPool单例实例
		
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0"); //将登录数设置为0

		//从配置文件中获取服务器的主机地址和RPC端口号，构成服务器的监听地址
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service; //创建ChatServiceImpl服务对象，负责处理RPC请求
		grpc::ServerBuilder builder; //创建gRPC服务器构建器
		//监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

		//构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		boost::asio::io_context io_context; //创建io_context对象，作为服务器的核心事件循环
		//创建一个信号集，监听SIGINT和SIGTERM信号
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		//当接收到SIGINT或SIGTERM信号时，调用lambda函数来停止io_context和线程池
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		auto port_str = cfg["SelfServer"]["Port"]; //从配置文件中获取端口号
		CServer s(io_context, atoi(port_str.c_str())); //创建服务器对象，传入io_context和端口号
		io_context.run(); //运行io_context，开始处理事件循环，直到接收到停止信号

		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name); //从Redis中删除登录数记录
		RedisMgr::GetInstance()->Close(); //关闭Redis连接池，释放资源
		grpc_server_thread.join(); //等待grpc服务线程结束
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

