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
	    Defer derfer ([server_name]() {
                RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
                RedisMgr::GetInstance()->Close();
            });

	    boost::asio::io_context io_context;
	    auto port_str = cfg["SelfServer"]["Port"];
	    //创建Cserver智能指针
	    auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));
	    pointer_server->StartTimer(); //启动定时器

		//从配置文件中获取服务器的主机地址和RPC端口号，构成服务器的监听地址
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service; //创建ChatServiceImpl服务对象，负责处理RPC请求
		grpc::ServerBuilder builder; //创建gRPC服务器构建器
		//监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
	    service.RegisterServer(pointer_server); //将Cserver注册给ChatServiceImpl服务对象，方便处理踢人请求

		//构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        //端口占用等原因可能使BuildAndStart返回空指针，启动失败时直接退出，不能继续调用Wait或Shutdown
        if (!server) {
            std::cerr << "Cannot start gRPC server on " << server_address << std::endl;
            return EXIT_FAILURE;
        }
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		//创建一个信号集，监听SIGINT和SIGTERM信号
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		//当接收到SIGINT或SIGTERM信号时，调用lambda函数来停止io_context和线程池
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

	    //将Cserver注册给逻辑类方便以后清除连接
	    LogicSystem::GetInstance()->SetServer(pointer_server);
		io_context.run(); //运行io_context，开始处理事件循环，直到接收到停止信号

		grpc_server_thread.join(); //等待grpc服务线程结束
	    pointer_server->StopTimer(); //停止定时器
        LogicSystem::GetInstance()->Stop(); //先结束业务处理，再关闭 Redis 和主 io_context
	    return 0;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

