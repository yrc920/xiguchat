#include "const.h"
#include "ConfigMgr.h"
#include "EmailSender.h"
#include "VerifyServiceImpl.h"

#include <boost/asio.hpp>

int main()
{
	auto& cfg = ConfigMgr::Inst(); //获取配置管理器实例（读取当前目录下的 config.ini）
	std::string server_address(cfg["VerifyServer"]["Host"] + ":" + cfg["VerifyServer"]["Port"]);

	try {
		EmailSender::InitSSL(); //SMTP 走 TLS，先初始化 OpenSSL 环境

		VerifyServiceImpl service; //gRPC 服务实现
		grpc::ServerBuilder builder;

		//监听端口并注册服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);

		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "Server listening on " << server_address << std::endl;

		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM); //捕获 Ctrl+C 与终止信号

		signals.async_wait([&server](const boost::system::error_code& error, int /*signal_number*/) {
			if (!error) {
				std::cout << "Shutting down server..." << std::endl;
				server->Shutdown(); //优雅关闭，等待在途 RPC 结束
			}
			});

		std::thread([&io_context]() { io_context.run(); }).detach(); //单独线程跑信号监听

		server->Wait(); //阻塞直到服务器关闭
		io_context.stop();
		EmailSender::UninitSSL();
	}
	catch (std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}
