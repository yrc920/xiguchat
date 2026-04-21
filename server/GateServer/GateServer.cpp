#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"

int main()
{
    try
    {
        unsigned short port = static_cast<unsigned short>(8080); //服务器监听的端口号
		net::io_context ioc{ 1 }; //使用一个线程来运行io_context
        //创建一个信号集, 用于捕捉SIGINT和SIGTERM信号, 以便优雅地关闭服务器
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM); 
		//异步等待信号, 当捕捉到信号时, 停止io_context
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
        });

		//创建服务器实例, 开始监听端口, 接受连接
        std::make_shared<CServer>(ioc, port)->Start();
		ioc.run(); //运行io_context, 处理异步事件
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
