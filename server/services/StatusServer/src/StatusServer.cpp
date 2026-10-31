#include "const.h"
#include "ConfigMgr.h"
#include "StatusServiceImpl.h"

int main(int argc, char** argv) {
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例
	//构建服务器地址
	std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);

	try {
		StatusServiceImpl service; //创建服务实例
		grpc::ServerBuilder builder; //创建gRPC服务器构建器

		//监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

		//构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        //端口占用等原因可能使BuildAndStart返回空指针，启动失败时直接退出，不能继续调用Wait或Shutdown
        if (!server) {
            std::cerr << "Cannot start gRPC server on " << server_address << std::endl;
            return EXIT_FAILURE;
        }
		std::cout << "Server listening on " << server_address << std::endl;

		boost::asio::io_context io_context; //创建Boost.Asio的io_context
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM); //创建signal_set用于捕获SIGINT

		//设置异步等待SIGINT信号
		signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
			if (!error) {
				std::cout << "Shutting down server..." << std::endl;
				server->Shutdown(); //优雅地关闭服务器
			}
		});

		std::thread signal_thread([&io_context]() { io_context.run(); }); //单独运行信号处理，主线程在Wait中等待服务结束

		server->Wait(); //等待服务器关闭
		io_context.stop();
        //信号线程引用了局部io_context，必须等线程退出后才能离开作用域，不能让线程访问已析构的对象
        signal_thread.join(); //停止io_context
	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}
