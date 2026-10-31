#include "IOContextPool.h"
#include <iostream>

//hardware_concurrency可能返回0，至少保留一个io_context和工作线程，避免轮询取用时访问空数组
IOContextPool::IOContextPool(std::size_t size) : _ioContexts(size == 0 ? 1 : size),
_works(size == 0 ? 1 : size), _nextIOContext(0)
{
	for (std::size_t i = 0; i < _ioContexts.size(); ++i) {
		//取出每个io_context的执行器，创建一个work对象，并把它放入works数组中
		_works[i] = std::make_unique<Work>(boost::asio::make_work_guard(_ioContexts[i].get_executor()));
	}

	//遍历多个io_context，创建多个线程，每个线程内部启动io_context
	for (std::size_t i = 0; i < _ioContexts.size(); ++i) {
		_threads.emplace_back([this, i]() {
			_ioContexts[i].run();
			});
	}
}

IOContextPool::~IOContextPool() {
	Stop(); //在析构函数中调用Stop方法，确保所有io_context都被正确停止，所有线程都被正确回收
	std::cout << "IOContextPool destruct" << std::endl;
}

boost::asio::io_context& IOContextPool::GetIOContext() {
	auto& service = _ioContexts[_nextIOContext++];
	//如果下一个io_context的索引已经超过了io_context数组的大小，就重置为0，继续从头开始分配io_context
	if (_nextIOContext == _ioContexts.size()) {
		_nextIOContext = 0;
	}
	return service;
}

void IOContextPool::Stop() {
    //由拥有线程池的线程调用Stop并等待工作线程退出，不能在被等待的工作线程自身调用
    //主流程显式停服后，析构函数还会调用一次，因此释放work和join都要允许重复执行
    //stop只让run尽快返回，不负责关闭socket，也不保证尚未执行的回调会继续被调用
    for (auto& context : _ioContexts) {
        context.stop(); //先停止事件循环，避免未完成的网络操作让线程一直等待
    }
    for (auto& work : _works) {
        work.reset(); //释放保活对象，空的unique_ptr再次reset也是安全的
    }
    for (auto& thread : _threads) {
        if (thread.joinable()) { //已经join过的线程不能重复join
            thread.join();
        }
    }
}
