#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"

void TestRedisMgr() {
	assert(RedisMgr::GetInstance()->Set("blogwebsite", "llfc.club"));
	std::string value = "";
	assert(RedisMgr::GetInstance()->Get("blogwebsite", value));
	assert(RedisMgr::GetInstance()->Get("nonekey", value) == false);
	assert(RedisMgr::GetInstance()->HSet("bloginfo", "blogwebsite", "llfc.club"));
	assert(RedisMgr::GetInstance()->HGet("bloginfo", "blogwebsite") != "");
	assert(RedisMgr::GetInstance()->ExistsKey("bloginfo"));
	assert(RedisMgr::GetInstance()->Del("bloginfo"));
	assert(RedisMgr::GetInstance()->Del("bloginfo"));
	assert(RedisMgr::GetInstance()->ExistsKey("bloginfo") == false);
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue1"));
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue2"));
	assert(RedisMgr::GetInstance()->LPush("lpushkey1", "lpushvalue3"));
	assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->RPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->LPop("lpushkey1", value));
	assert(RedisMgr::GetInstance()->LPop("lpushkey2", value) == false);
}

int main()
{
	TestRedisMgr();

	auto& gCfgMgr = ConfigMgr::Inst(); //获取配置管理器实例, 用于读取配置文件中的数据
	//从配置管理器中获取GateServer的Port配置项, 并将其转换为unsigned short类型
    std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());

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
		std::cout << "Server is listening on port " << port << std::endl;
		ioc.run(); //运行io_context, 处理异步事件
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
