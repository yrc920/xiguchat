#include "IOContextPool.h"
#include <iostream>

IOContextPool::IOContextPool(std::size_t size) : _ioContexts(size),
_works(size), _nextIOContext(0) {
	for (std::size_t i = 0; i < size; ++i) {
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
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : _works) {
		//先把io_context停止掉，这样就不会再有新的事件被添加到io_context中，已经绑定的事件也会被取消掉
		work->get_executor().context().stop();
		work.reset(); //重置work对象，释放对io_context的引用，让io_context被回收, 线程也会退出
	}

	//等待所有线程完成
	for (auto& t : _threads) {
		t.join();
	}
}
