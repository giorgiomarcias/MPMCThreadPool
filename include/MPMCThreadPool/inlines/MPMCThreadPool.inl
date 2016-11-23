// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#include <MPMCThreadPool/MPMCThreadPool.hpp>

namespace mpmc_tp {

	////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline MPMCThreadPool<SIZE>::MPMCThreadPool() : _active(false)
	{ }

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// DESTRUCTOR
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline MPMCThreadPool<SIZE>::~MPMCThreadPool()
	{
		stop();
	}



	////////////////////////////////////////////////////////////////////////
	// ACCESS METHODS
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline constexpr std::size_t MPMCThreadPool<SIZE>::size() const
	{
		return COMPILETIME_SIZE;
	}

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// METHODS FOR TASKS
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::start()
	{
		if (_active.load(std::memory_order::memory_order_relaxed))
			return;
		_active.store(true, std::memory_order::memory_order_relaxed);
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			_threads[i] = std::thread(&MPMCThreadPool<SIZE>::threadJob, this);
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::interrupt()
	{
		_active.store(false, std::memory_order::memory_order_relaxed);
		_condVar.notify_all();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::stop()
	{
		interrupt();
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			if (_threads[i].joinable())
				_threads[i].join();
		_taskQueue = ConcurrentQueue<SimpleTaskType>();
	}

	template < std::size_t SIZE >
	inline ProducerToken MPMCThreadPool<SIZE>::newProducerToken()
	{
		return ProducerToken(_taskQueue);
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushWork(const SimpleTaskType &task)
	{
		_taskQueue.enqueue(task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushWork(SimpleTaskType &&task)
	{
		_taskQueue.enqueue(std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushWork(const ProducerToken &token, const SimpleTaskType &task)
	{
		_taskQueue.enqueue(token, task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushWork(const ProducerToken &token, SimpleTaskType &&task)
	{
		_taskQueue.enqueue(token, std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::threadJob()
	{
		SimpleTaskType task;
		while (_active.load(std::memory_order::memory_order_relaxed)) {
			if (_taskQueue.try_dequeue(task)) {
				if (task)
					task();
			} else {
				std::unique_lock<std::mutex> lock(_mutex);
				_condVar.wait(lock, [this]()->bool{
					return !_active.load(std::memory_order::memory_order_relaxed) || _taskQueue.size_approx() > 0;
				});
			}
		}
	}

	////////////////////////////////////////////////////////////////////////

}
