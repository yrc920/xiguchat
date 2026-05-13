#include "LogicSystem.h"
#include <thread>
#include <mutex>
#include <csignal>
#include "IOContextPool.h"
#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
	try {
		auto& cfg = ConfigMgr::Inst(); //加载配置文件
		auto port_str = cfg["SelfServer"]["Port"]; //从配置文件中获取端口号

		auto pool = IOContextPool::GetInstance(); //获取IOContextPool单例实例
		boost::asio::io_context io_context; //创建io_context对象，作为服务器的核心事件循环
		//创建一个信号集，监听SIGINT和SIGTERM信号
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		//当接收到SIGINT或SIGTERM信号时，调用lambda函数来停止io_context和线程池
		signals.async_wait([&io_context, pool](auto, auto) {
			io_context.stop();
			pool->Stop();
			});
		
		CServer s(io_context, atoi(port_str.c_str())); //创建服务器对象，传入io_context和端口号
		io_context.run(); //运行io_context，开始处理事件循环，直到接收到停止信号
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

