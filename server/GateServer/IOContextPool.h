#pragma once
// 线程池, 用于管理多个io_context对象, 每个io_context对象在一个独立的线程中运行
//

#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

class IOContextPool : public Singleton<IOContextPool>
{
	friend Singleton<IOContextPool>; //使基类创建实例时可以访问私有构造函数
public:
	using IOContext = boost::asio::io_context;
	//使用executor_work_guard来保持io_context的运行状态, 防止io_context在没有任务时退出
	using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using WorkPtr = std::unique_ptr<Work>;

	~IOContextPool(); //使基类析构时可以访问派生类的析构函数
	//禁止拷贝构造和赋值操作
	IOContextPool(const IOContextPool&) = delete;
	IOContextPool& operator=(const IOContextPool&) = delete;

	//使用 round-robin 的方式返回一个 io_context
	boost::asio::io_context& GetIOContext();
	void Stop(); //停止所有 io_context，等待所有线程完成

private:
	//线程池的大小默认为cpu核心数
	IOContextPool(std::size_t size = std::thread::hardware_concurrency());
	//每个io_context对象都需要一个work对象来保持其运行状态, 跑在一个独立的线程中运行
	std::vector<IOContext> _ioContexts;
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;

	std::size_t _nextIOContext; //下一个io_context的索引
};
