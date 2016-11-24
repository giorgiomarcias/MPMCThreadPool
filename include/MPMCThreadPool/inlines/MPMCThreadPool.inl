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
	inline MPMCThreadPool<SIZE>::MPMCThreadPool() : _active(true)
	{
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			_threads[i] = std::thread(&MPMCThreadPool<SIZE>::threadJob, this);
	}

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// DESTRUCTOR
	////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline MPMCThreadPool<SIZE>::~MPMCThreadPool()
	{
		_active.store(false, std::memory_order::memory_order_relaxed);
		_condVar.notify_all();
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			if (_threads[i].joinable())
				_threads[i].join();
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
	inline ProducerToken MPMCThreadPool<SIZE>::newProducerToken()
	{
		return ProducerToken(_taskQueue);
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushTask(const SimpleTaskType &task)
	{
		_taskQueue.enqueue(task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushTask(SimpleTaskType &&task)
	{
		_taskQueue.enqueue(std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushTask(const ProducerToken &token, const SimpleTaskType &task)
	{
		_taskQueue.enqueue(token, task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::pushTask(const ProducerToken &token, SimpleTaskType &&task)
	{
		_taskQueue.enqueue(token, std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < std::size_t SIZE > template < class It >
	inline void MPMCThreadPool<SIZE>::pushTasks(It first, It last)
	{
		std::size_t n = std::distance(first, last);
		if (n == 0)
			return;
		_taskQueue.enqueue_bulk(std::forward<It>(first), n);
		if (n > 1)
			_condVar.notify_all();
		else
			_condVar.notify_one();
	}

	template < std::size_t SIZE > template < class It >
	inline void MPMCThreadPool<SIZE>::pushTasks(const ProducerToken &token, It first, It last)
	{
		std::size_t n = std::distance(first, last);
		if (n == 0)
			return;
		_taskQueue.enqueue_bulk(token, std::forward<It>(first), n);
		if (n > 1)
			_condVar.notify_all();
		else
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





	namespace internal {

		inline TaskPackBase::TaskPackBase(const std::size_t size) : _tasks(size)
		{ }

		inline std::size_t TaskPackBase::size() const
		{
			return _tasks.size();
		}

		inline TaskPackBase::iterator TaskPackBase::begin()
		{
			return _tasks.begin();
		}

		inline TaskPackBase::const_iterator TaskPackBase::begin() const
		{
			return _tasks.begin();
		}

		inline TaskPackBase::move_iterator TaskPackBase::moveBegin()
		{
			return std::make_move_iterator(_tasks.begin());
		}

		inline TaskPackBase::iterator TaskPackBase::end()
		{
			return _tasks.end();
		}

		inline TaskPackBase::const_iterator TaskPackBase::end() const
		{
			return _tasks.end();
		}

		inline TaskPackBase::move_iterator TaskPackBase::moveEnd()
		{
			return std::make_move_iterator(_tasks.end());
		}

	}



	inline TaskPackTraitsLockFree::TaskPackTraitsLockFree(const std::size_t size) : _size(size), _nCompletedTasks(0)
	{ }

	inline std::size_t TaskPackTraitsLockFree::nCompletedTasks() const
	{
		return _nCompletedTasks.load(std::memory_order::memory_order_relaxed);
	}

	inline void TaskPackTraitsLockFree::signalTaskComplete(const std::size_t)
	{
		_nCompletedTasks.fetch_add(1, std::memory_order::memory_order_relaxed);
	}

	inline void TaskPackTraitsLockFree::wait()
	{
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			;
	}

	inline SimpleTaskType TaskPackTraitsLockFree::createWaitTask()
	{
		return SimpleTaskType();
	}



	template < class R, class TaskPackTraits > template < class ...Args >
	inline TaskPack<R, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<args>(args)...), _results(size, R())
	{ }

	template < class R, class TaskPackTraits > template < class F, class ...Args >
	inline void TaskPack<R, TaskPackTraits>::setTaskAt(const std::size_t i, F &&f, Args &&...args)
	{
		static_assert(std::is_convertible<typename std::result_of<F(Args...)>::type, R>::value, "Result type of callable object must be same of TaskPack template parameter.");
		auto g = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		_tasks.at(i) = [i, g, this](){
			_results.at(i) = g();
			TaskPackTraits::signalTaskComplete(i);
		};
	}

	template < class R, class TaskPackTraits >
	inline const R & TaskPack<R, TaskPackTraits>::resultAt(const std::size_t i) const
	{
		return _results.at(i);
	}



	template < class TaskPackTraits > template < class ...Args >
	inline TaskPack<void, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<args>(args)...)
	{ }

	template < class TaskPackTraits > template < class F, class ...Args >
	inline void TaskPack<void, TaskPackTraits>::setTaskAt(const std::size_t i, F &&f, Args &&...args)
	{
		static_assert(std::is_void<typename std::result_of<F(Args...)>::type>::value, "Result type of callable object must be same of TaskPack template parameter.");
		auto g = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		_tasks.at(i) = [i, g, this](){
			g();
			TaskPackTraits::signalTaskComplete(i);
		};
	}

}
