// Copyright (c) 2016 Giorgio Marcias
//
// This source code is subject to the simplified BSD license.
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#include <MPMCThreadPool/MPMCThreadPool.hpp>
#include <future>

namespace mpmc_tp {

	////////////////////////////////////////////////////////////////////////////
	// MPMCThreadPool METHODS
	////////////////////////////////////////////////////////////////////////////

	inline std::size_t MPMCThreadPool::DEFAULT_SIZE()
	{
		return std::thread::hardware_concurrency();
	}

	inline MPMCThreadPool::MPMCThreadPool() : _threads(MPMCThreadPool::DEFAULT_SIZE()), _actives(MPMCThreadPool::DEFAULT_SIZE()), _active(true)
	{
		_flag.clear();
		for (std::size_t i = 0; i < _actives.size(); ++i)
			_actives.at(i).store(true, std::memory_order::memory_order_relaxed);
		for (std::size_t i = 0; i < _threads.size(); ++i)
			_threads.at(i) = std::thread(&MPMCThreadPool::threadJob, this, std::ref(_actives.at(i)));
	}

	inline MPMCThreadPool::MPMCThreadPool(const std::size_t size) : _threads(size), _actives(size), _active(true)
	{
		_flag.clear();
		for (std::size_t i = 0; i < _actives.size(); ++i)
			_actives.at(i).store(true, std::memory_order::memory_order_relaxed);
		for (std::size_t i = 0; i < _threads.size(); ++i)
			_threads.at(i) = std::thread(&MPMCThreadPool::threadJob, this, std::ref(_actives.at(i)));
	}

	inline MPMCThreadPool::~MPMCThreadPool()
	{
		while (_flag.test_and_set())
			;
		_active.store(false, std::memory_order::memory_order_relaxed);
		_condVar.notify_all();
		for (std::size_t i = 0; i < _threads.size(); ++i)
			if (_threads.at(i).joinable())
				_threads.at(i).join();
		_flag.clear();
	}

	inline std::size_t MPMCThreadPool::size() const
	{
		while (_flag.test_and_set())
			;
		std::size_t size = _threads.size();
		_flag.clear();
		return size;
	}

	inline void MPMCThreadPool::expand(const std::size_t n)
	{
		while (_flag.test_and_set())
			;
		std::size_t oldSize = _threads.size();
		_threads.resize(oldSize + n);
		_actives.resize(oldSize + n);
		for (std::size_t i = 0; i < n; ++i)
			_actives.at(oldSize + i).store(true, std::memory_order::memory_order_relaxed);
		for (std::size_t i = 0; i < n; ++i)
			_threads.at(oldSize + i) = std::thread(&MPMCThreadPool::threadJob, this, std::ref(_actives.at(oldSize + i)));
		_flag.clear();
	}

	inline void MPMCThreadPool::shrink(const std::size_t n)
	{
		while (_flag.test_and_set())
			;
		std::size_t newSize = _threads.size() - std::min(_threads.size(), n);
		for (std::size_t i = newSize; i < _actives.size(); ++i)
			_actives.at(i).store(false, std::memory_order::memory_order_relaxed);
		_condVar.notify_all();
		for (std::size_t i = newSize; i < _threads.size(); ++i)
			if (_threads.at(i).joinable())
				_threads.at(i).join();
		_threads.resize(newSize);
		_actives.resize(newSize);
		_flag.clear();
	}

	inline ProducerToken MPMCThreadPool::newProducerToken()
	{
		return ProducerToken(_taskQueue);
	}

	inline void MPMCThreadPool::submitTask(const SimpleTaskType &task)
	{
		_taskQueue.enqueue(task);
		_condVar.notify_one();
	}

	inline void MPMCThreadPool::submitTask(SimpleTaskType &&task)
	{
		_taskQueue.enqueue(std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	inline void MPMCThreadPool::submitTask(const ProducerToken &token, const SimpleTaskType &task)
	{
		_taskQueue.enqueue(token, task);
		_condVar.notify_one();
	}

	inline void MPMCThreadPool::submitTask(const ProducerToken &token, SimpleTaskType &&task)
	{
		_taskQueue.enqueue(token, std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < class It >
	inline void MPMCThreadPool::submitTasks(It first, It last)
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

	template < class It >
	inline void MPMCThreadPool::submitTasks(const ProducerToken &token, It first, It last)
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

	inline void MPMCThreadPool::threadJob(std::atomic_bool &active)
	{
		SimpleTaskType task;
		while (_active.load(std::memory_order::memory_order_relaxed) && active.load(std::memory_order::memory_order_relaxed)) {
			if (_taskQueue.try_dequeue(task)) {
				if (task)
					task();
			} else {
				std::unique_lock<std::mutex> lock(_mutex);
				_condVar.wait(lock, [this, &active]()->bool{
					return !_active.load(std::memory_order::memory_order_relaxed) || !active.load(std::memory_order::memory_order_relaxed) || _taskQueue.size_approx() > 0;
				});
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////






	////////////////////////////////////////////////////////////////////////////
	// TRAITS
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////
	// TaskPackTraitsLockFree METHODS
	////////////////////////////////////////////////////////////////////////

	inline TaskPackTraitsLockFree::TaskPackTraitsLockFree(const std::size_t size) : _size(size), _nCompletedTasks(0), _interval(0)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsLockFree::TaskPackTraitsLockFree(const std::size_t size, const std::chrono::duration<Rep, Period> &interval) : _size(size), _nCompletedTasks(0), _interval(interval)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsLockFree::TaskPackTraitsLockFree(const std::size_t size, std::chrono::duration<Rep, Period> &&interval) : _size(size), _nCompletedTasks(0), _interval(std::forward<std::chrono::duration<Rep, Period>>(interval))
	{ }

	inline void TaskPackTraitsLockFree::setTraitsSize(const std::size_t size)
	{
		_size = size;
	}

	template < class Rep, class Period >
	inline void TaskPackTraitsLockFree::setInterval(const std::chrono::duration<Rep, Period> &interval)
	{
		_interval = interval;
	}

	template < class Rep, class Period >
	inline void TaskPackTraitsLockFree::setInterval(std::chrono::duration<Rep, Period> &&interval)
	{
		_interval = std::forward<std::chrono::duration<Rep, Period>>(interval);
	}

	template < class C, class ...Args >
	inline void TaskPackTraitsLockFree::setCallback(C &&c, Args &&...args)
	{
		_callback = std::bind(std::forward<C>(c), std::placeholders::_1, std::forward<Args>(args)...);
	}

	inline void TaskPackTraitsLockFree::signalTaskComplete(const std::size_t i)
	{
		_nCompletedTasks.fetch_add(1, std::memory_order::memory_order_relaxed);
		if (_callback)
			_callback(i);
	}

	inline std::size_t TaskPackTraitsLockFree::nCompletedTasks() const
	{
		return _nCompletedTasks.load(std::memory_order::memory_order_relaxed);
	}

	inline void TaskPackTraitsLockFree::wait() const
	{
		waitComplete();
	}

	inline void TaskPackTraitsLockFree::waitComplete() const
	{
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			if (_interval.count() > 0)
				std::this_thread::sleep_for(_interval);
	}

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// TaskPackTraitsBlockingWait METHODS
	////////////////////////////////////////////////////////////////////////

	inline TaskPackTraitsBlockingWait::TaskPackTraitsBlockingWait(const std::size_t size) : TaskPackTraitsLockFree(size), _completed(false)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsBlockingWait::TaskPackTraitsBlockingWait(const std::size_t size, const std::chrono::duration<Rep, Period> &interval) : TaskPackTraitsLockFree(size, interval), _completed(false)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsBlockingWait::TaskPackTraitsBlockingWait(const std::size_t size, std::chrono::duration<Rep, Period> &&interval) : TaskPackTraitsLockFree(size, std::forward<std::chrono::duration<Rep, Period>>(interval)), _completed(false)
	{ }

	inline void TaskPackTraitsBlockingWait::wait() const
	{
		std::unique_lock<std::mutex> lock(_waitMutex);
		_waitCondVar.wait(lock, [this]()->bool{ return _completed.load(std::memory_order::memory_order_acquire); });
	}

	inline void TaskPackTraitsBlockingWait::waitComplete() const
	{
		TaskPackTraitsLockFree::waitComplete();
		_completed.store(true, std::memory_order::memory_order_release);
		_waitCondVar.notify_all();
	}

	////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////
	// TaskPackTraitsBlocking METHODS
	////////////////////////////////////////////////////////////////////////////

	inline TaskPackTraitsBlocking::TaskPackTraitsBlocking(const std::size_t size) : TaskPackTraitsBlockingWait(size)
	{ }

	inline void TaskPackTraitsBlocking::signalTaskComplete(const std::size_t i)
	{
		TaskPackTraitsBlockingWait::signalTaskComplete(i);
		_signalCondVar.notify_all();
	}

	inline void TaskPackTraitsBlocking::waitComplete() const
	{
		std::unique_lock<std::mutex> lock(_signalMutex);
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			_signalCondVar.wait(lock, [this]()->bool{ return _nCompletedTasks.load(std::memory_order::memory_order_relaxed) >= _size; });
		_completed.store(true, std::memory_order::memory_order_release);
		_waitCondVar.notify_all();
	}

	////////////////////////////////////////////////////////////////////////////






	////////////////////////////////////////////////////////////////////////////
	// INTERNAL STUFF
	////////////////////////////////////////////////////////////////////////////

	namespace internal {

		////////////////////////////////////////////////////////////////////////
		// TaskPackBase METHODS
		////////////////////////////////////////////////////////////////////////

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

		////////////////////////////////////////////////////////////////////////

	}

	////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////
	// TaskPack METHODS
	////////////////////////////////////////////////////////////////////////////

	template < class R, class TaskPackTraits > template < class ...Args >
	inline TaskPack<R, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<Args>(args)...), _results(size, R()), _waitTaskIndex(std::numeric_limits<std::size_t>::max())
	{ }

	template < class R, class TaskPackTraits > template < class F, class ...Args >
	inline void TaskPack<R, TaskPackTraits>::setTaskAt(const std::size_t i, F &&f, Args &&...args)
	{
		static_assert(std::is_convertible<typename std::result_of<F(Args...)>::type, R>::value, "Result type of callable object must be same of TaskPack template parameter.");
		static_assert(std::is_void<decltype(std::declval<TaskPack<R, TaskPackTraits>>().signalTaskComplete(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void signalTaskComplete(std::size_t)' method.");
		auto g = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		_tasks.at(i) = [i, g, this](){
			_results.at(i) = g();
			this->signalTaskComplete(i);
		};
	}

	template < class R, class TaskPackTraits >
	inline void TaskPack<R, TaskPackTraits>::setWaitTaskAt(const std::size_t i)
	{
		static_assert(std::is_void<decltype(std::declval<TaskPack<R, TaskPackTraits>>().waitComplete())>::value, "The TaskPackTraits template parameter must have a 'void waitComplete()' method.");
		static_assert(std::is_void<decltype(std::declval<TaskPack<void, TaskPackTraits>>().setTraitsSize(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void setTraitsSize(std::size_t)' method.");
		if (_waitTaskIndex < _tasks.size())
			throw std::logic_error("The wait task can not be set more than once.");
		if (_tasks.at(i))
			throw std::logic_error("The wait task can not be set into a non-empty bin.");
		_tasks.at(i) = std::bind(&TaskPack<R, TaskPackTraits>::waitComplete, this);
		this->setTraitsSize(_tasks.size() - 1);
		_waitTaskIndex = i;
	}

	template < class R, class TaskPackTraits >
	inline const R & TaskPack<R, TaskPackTraits>::resultAt(const std::size_t i) const
	{
		return _results.at(i);
	}



	template < class TaskPackTraits > template < class ...Args >
	inline TaskPack<void, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<Args>(args)...), _waitTaskIndex(std::numeric_limits<std::size_t>::max())
	{ }

	template < class TaskPackTraits > template < class F, class ...Args >
	inline void TaskPack<void, TaskPackTraits>::setTaskAt(const std::size_t i, F &&f, Args &&...args)
	{
		static_assert(std::is_void<typename std::result_of<F(Args...)>::type>::value, "Result type of callable object must be same of TaskPack template parameter.");
		static_assert(std::is_void<decltype(std::declval<TaskPack<void, TaskPackTraits>>().signalTaskComplete(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void signalTaskComplete(std::size_t)' method.");
		auto g = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		_tasks.at(i) = [i, g, this](){
			g();
			this->signalTaskComplete(i);
		};
	}

	template < class TaskPackTraits >
	inline void TaskPack<void, TaskPackTraits>::setWaitTaskAt(const std::size_t i)
	{
		static_assert(std::is_void<decltype(std::declval<TaskPack<void, TaskPackTraits>>().waitComplete())>::value, "The TaskPackTraits template parameter must have a 'void waitComplete()' method.");
		static_assert(std::is_void<decltype(std::declval<TaskPack<void, TaskPackTraits>>().setTraitsSize(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void setTraitsSize(std::size_t)' method.");
		if (_waitTaskIndex < _tasks.size())
			throw std::logic_error("The wait task can not be set more than once.");
		if (_tasks.at(i))
			throw std::logic_error("The wait task can not be set into a non-empty bin.");
		_tasks.at(i) = std::bind(&TaskPack<void, TaskPackTraits>::waitComplete, this);
		this->setTraitsSize(_tasks.size() - 1);
		_waitTaskIndex = i;
	}

	////////////////////////////////////////////////////////////////////////////

}
