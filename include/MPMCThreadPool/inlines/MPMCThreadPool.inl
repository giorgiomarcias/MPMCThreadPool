// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#include <MPMCThreadPool/MPMCThreadPool.hpp>
#include <future>

namespace mpmc_tp {

	////////////////////////////////////////////////////////////////////////////
	// MPMCThreadPool METHODS
	////////////////////////////////////////////////////////////////////////////

	template < std::size_t SIZE >
	inline MPMCThreadPool<SIZE>::MPMCThreadPool() : _active(true)
	{
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			_threads[i] = std::thread(&MPMCThreadPool<SIZE>::threadJob, this);
	}

	template < std::size_t SIZE >
	inline MPMCThreadPool<SIZE>::~MPMCThreadPool()
	{
		_active.store(false, std::memory_order::memory_order_relaxed);
		_condVar.notify_all();
		for (std::size_t i = 0; i < COMPILETIME_SIZE; ++i)
			if (_threads[i].joinable())
				_threads[i].join();
	}

	template < std::size_t SIZE >
	inline constexpr std::size_t MPMCThreadPool<SIZE>::size() const
	{
		return COMPILETIME_SIZE;
	}

	template < std::size_t SIZE >
	inline ProducerToken MPMCThreadPool<SIZE>::newProducerToken()
	{
		return ProducerToken(_taskQueue);
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::postTask(const SimpleTaskType &task)
	{
		_taskQueue.enqueue(task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::postTask(SimpleTaskType &&task)
	{
		_taskQueue.enqueue(std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::postTask(const ProducerToken &token, const SimpleTaskType &task)
	{
		_taskQueue.enqueue(token, task);
		_condVar.notify_one();
	}

	template < std::size_t SIZE >
	inline void MPMCThreadPool<SIZE>::postTask(const ProducerToken &token, SimpleTaskType &&task)
	{
		_taskQueue.enqueue(token, std::forward<SimpleTaskType>(task));
		_condVar.notify_one();
	}

	template < std::size_t SIZE > template < class It >
	inline void MPMCThreadPool<SIZE>::postTasks(It first, It last)
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
	inline void MPMCThreadPool<SIZE>::postTasks(const ProducerToken &token, It first, It last)
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
	inline void TaskPackTraitsLockFree::setCallback(const C &c, const Args &...args)
	{
		_callback = std::bind(c, std::placeholders::_1, args...);
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
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			;
	}

	////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////
	// TaskPackTraitsBlockWait METHODS
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
		std::unique_lock<std::mutex> lock(_mutex);
		_condVar.wait(lock, [this]()->bool{ return _completed.load(std::memory_order::memory_order_acquire); });
	}

	////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////
	// TaskPackTraitsSimple METHODS
	////////////////////////////////////////////////////////////////////////////

	template < class R >
	inline TaskPackTraitsSimple<R>::TaskPackTraitsSimple(const std::size_t size) : TaskPackTraitsBlockingWait(size)
	{ }

	template < class R > template < class Rep, class Period >
	inline TaskPackTraitsSimple<R>::TaskPackTraitsSimple(const std::size_t size, const std::chrono::duration<Rep, Period> &interval) : TaskPackTraitsBlockingWait(size, interval)
	{ }

	template < class R > template < class Rep, class Period >
	inline TaskPackTraitsSimple<R>::TaskPackTraitsSimple(const std::size_t size, std::chrono::duration<Rep, Period> &&interval) : TaskPackTraitsBlockingWait(size, std::forward<std::chrono::duration<Rep, Period>>(interval))
	{ }

	template < class R > template < class F, class ...Args >
	inline void TaskPackTraitsSimple<R>::setReduce(F &&f, Args &&...args)
	{
		_reduce = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	}

	template < class R >
	inline const R & TaskPackTraitsSimple<R>::waitCompletionAndReduce()
	{
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			if (_interval.count() > 0)
				std::this_thread::sleep_for(_interval);
		if (_reduce)
			_reducedResult = _reduce();
		else
			_reducedResult = R();
		_completed.store(true, std::memory_order::memory_order_release);
		_condVar.notify_all();
		return _reducedResult;
	}

	template < class R >
	inline std::function<R()> TaskPackTraitsSimple<R>::createWaitTask()
	{
		if (_size == 0)
			return std::function<R()>();
		--_size;
		return std::bind(&TaskPackTraitsSimple<R>::waitCompletionAndReduce, this);
	}

	template < class R >
	inline const R & TaskPackTraitsSimple<R>::getResult() const
	{
		TaskPackTraitsBlockingWait::wait();
		return _reducedResult;
	}



	inline TaskPackTraitsSimple<void>::TaskPackTraitsSimple(const std::size_t size) : TaskPackTraitsBlockingWait(size)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsSimple<void>::TaskPackTraitsSimple(const std::size_t size, const std::chrono::duration<Rep, Period> &interval) : TaskPackTraitsBlockingWait(size, interval)
	{ }

	template < class Rep, class Period >
	inline TaskPackTraitsSimple<void>::TaskPackTraitsSimple(const std::size_t size, std::chrono::duration<Rep, Period> &&interval) : TaskPackTraitsBlockingWait(size, std::forward<std::chrono::duration<Rep, Period>>(interval))
	{ }

	inline void TaskPackTraitsSimple<void>::waitCompletion()
	{
		while (_nCompletedTasks.load(std::memory_order::memory_order_relaxed) < _size)
			if (_interval.count() > 0)
				std::this_thread::sleep_for(_interval);
		_completed.store(true, std::memory_order::memory_order_release);
		_condVar.notify_all();
	}

	inline std::function<void()> TaskPackTraitsSimple<void>::createWaitTask()
	{
		if (_size == 0)
			return std::function<void()>();
		--_size;
		return std::bind(&TaskPackTraitsSimple<void>::waitCompletion, this);
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
	inline TaskPack<R, TaskPackTraits>::TaskPack(const std::size_t size, const Args &...args) : internal::TaskPackBase(size), TaskPackTraits(size, args...), _results(size, R())
	{ }

	template < class R, class TaskPackTraits > template < class ...Args >
	inline TaskPack<R, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<Args>(args)...), _results(size, R())
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
	inline TaskPack<void, TaskPackTraits>::TaskPack(const std::size_t size, const Args &...args) : internal::TaskPackBase(size), TaskPackTraits(size, args...)
	{ }

	template < class TaskPackTraits > template < class ...Args >
	inline TaskPack<void, TaskPackTraits>::TaskPack(const std::size_t size, Args &&...args) : internal::TaskPackBase(size), TaskPackTraits(size, std::forward<Args>(args)...)
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

	////////////////////////////////////////////////////////////////////////////

}
