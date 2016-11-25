// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#ifndef MPMCThreadPool_hpp
#define MPMCThreadPool_hpp

#include <concurrentqueue/concurrentqueue.h>
#include <vector>
#include <future>

namespace mpmc_tp {

	using namespace moodycamel;



	////////////////////////////////////////////////////////////////////////////
	// NAMESPACE-LEVEL DEFINITIONS
	////////////////////////////////////////////////////////////////////////////

	using SimpleTaskType = std::function<void()>;

	////////////////////////////////////////////////////////////////////////////



	/// The MPMCThreadPool class
	template < std::size_t SIZE >
	class MPMCThreadPool {
	public:

		static_assert(SIZE > 0UL, "Invalid thread pool size: it must own at least one thread.");

		////////////////////////////////////////////////////////////////////////
		// DEFINITIONS
		////////////////////////////////////////////////////////////////////////

		static constexpr std::size_t COMPILETIME_SIZE = SIZE;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Default constructor.
		 */
		inline MPMCThreadPool();


		/**
		 *    @brief Copy constructor.
		 */
		MPMCThreadPool(const MPMCThreadPool &other) = delete;

		/**
		 *    @brief Move constructor.
		 */
		MPMCThreadPool(MPMCThreadPool &&other) = default;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// DESTRUCTOR
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Default destructor.
		 */
		inline ~MPMCThreadPool();

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Copy assignment operator.
		 */
		MPMCThreadPool & operator=(const MPMCThreadPool &other) = delete;

		/**
		 *    @brief Move assignment operator.
		 */
		MPMCThreadPool & operator=(MPMCThreadPool &&other) = default;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ACCESS METHODS
		////////////////////////////////////////////////////////////////////////

		inline constexpr std::size_t size() const;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// METHODS FOR TASKS
		////////////////////////////////////////////////////////////////////////

		inline ProducerToken newProducerToken();

		inline void pushTask(const SimpleTaskType &task);

		inline void pushTask(SimpleTaskType &&task);

		inline void pushTask(const ProducerToken &token, const SimpleTaskType &task);

		inline void pushTask(const ProducerToken &token, SimpleTaskType &&task);

		template < class It >
		inline void pushTasks(It first, It last);

		template < class It >
		inline void pushTasks(const ProducerToken &token, It first, It last);

		////////////////////////////////////////////////////////////////////////


	private:
		////////////////////////////////////////////////////////////////////////
		// PRIVATE DEFINITIONS
		////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// PRIVATE METHODS
		////////////////////////////////////////////////////////////////////////

		inline void threadJob();

		////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////
		// PRIVATE MEMBERS
		////////////////////////////////////////////////////////////////////////

		std::array<std::thread, SIZE>    _threads;  ///< Array of thread objects.

		ConcurrentQueue<SimpleTaskType>  _taskQueue;///< Queue of tasks.

		std::atomic_bool                 _active;   ///< Signal for stopping the threads.

		std::mutex                       _mutex;    ///< Mutex for allowing thread suspension when the queue is empty.
		std::condition_variable          _condVar;  ///< Condition variable for thread wakeup when the queue is no more empty.

		////////////////////////////////////////////////////////////////////////

	};



	namespace internal {

		class TaskPackBase {
		protected:
			template < class T >
			using Container = std::vector<T>;
			using SimpleTaskContainer = Container<SimpleTaskType>;

		public:
			using iterator       = SimpleTaskContainer::iterator;
			using const_iterator = SimpleTaskContainer::const_iterator;
			using move_iterator  = std::move_iterator<iterator>;

			inline TaskPackBase(const std::size_t size);

			TaskPackBase(const TaskPackBase &) = delete;
			TaskPackBase(TaskPackBase &&) = delete;

			TaskPackBase & operator=(const TaskPackBase &) = delete;
			TaskPackBase & operator=(TaskPackBase &&) = delete;

			inline std::size_t size() const;

			inline iterator begin();
			inline const_iterator begin() const;
			inline move_iterator moveBegin();

			inline iterator end();
			inline const_iterator end() const;
			inline move_iterator moveEnd();

			inline const SimpleTaskType & at(const std::size_t i) const;
			inline SimpleTaskType & at(const std::size_t i);

			inline const SimpleTaskType & operator[](const std::size_t i) const;
			inline SimpleTaskType & operator[](const std::size_t i);

		protected:
			SimpleTaskContainer  _tasks;
		};
	}



	class TaskPackTraitsSimple {
	public:
		inline TaskPackTraitsSimple(const std::size_t size);

		inline TaskPackTraitsSimple(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple(TaskPackTraitsSimple &&) = delete;

		inline TaskPackTraitsSimple & operator=(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple & operator=(TaskPackTraitsSimple &&) = delete;

		inline void signalTaskComplete(const std::size_t);

		inline std::size_t nCompletedTasks() const;

		inline void wait();

	private:
		std::size_t         _size;
		std::atomic_size_t  _nCompletedTasks;
	};



	template < class R >
	class TaskPackTraitsSimpleBlocking {
	public:
		inline TaskPackTraitsSimpleBlocking(const std::size_t size);

		template < class Rep, class Period >
		inline TaskPackTraitsSimpleBlocking(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

		template < class Rep, class Period >
		inline TaskPackTraitsSimpleBlocking(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

		inline TaskPackTraitsSimpleBlocking(const TaskPackTraitsSimpleBlocking &) = delete;
		inline TaskPackTraitsSimpleBlocking(TaskPackTraitsSimpleBlocking &&) = delete;

		inline TaskPackTraitsSimpleBlocking & operator=(const TaskPackTraitsSimpleBlocking &) = delete;
		inline TaskPackTraitsSimpleBlocking & operator=(TaskPackTraitsSimpleBlocking &&) = delete;

		template < class Rep, class Period >
		inline void setInterval(const std::chrono::duration<Rep, Period> &interval);

		inline void signalTaskComplete(const std::size_t);

		inline std::size_t nCompletedTasks() const;

		template < class F, class ...Args >
		inline void setReduce(F &&f, Args &&...args);

		inline std::function<R()> createWaitTask();

		inline void wait();

		inline R getResult() const;

	private:
		inline R waitJob();
		inline void prepareFuture();

		std::size_t               _size;
		std::atomic_size_t        _nCompletedTasks;
		std::chrono::nanoseconds  _interval;
		std::function<R()>        _reduce;
		std::future<R>            _result;
	};



	using TaskPackTraitsDefault = TaskPackTraitsSimple;



	template < class R, class TaskPackTraits = TaskPackTraitsDefault >
	class TaskPack : public internal::TaskPackBase, public TaskPackTraits {

		static_assert(std::is_void<decltype(std::declval<TaskPackTraits>().signalTaskComplete(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void signalTaskComplete(const std::size_t)' member function.");

	protected:
		using internal::TaskPackBase::Container;

	public:
		template < class ...Args >
		inline TaskPack(const std::size_t size, const Args &...args);

		template < class ...Args >
		inline TaskPack(const std::size_t size, Args &&...args);

		TaskPack(const TaskPack &) = delete;
		TaskPack(TaskPack &&) = delete;

		TaskPack & operator=(const TaskPack &) = delete;
		TaskPack & operator=(TaskPack &&) = delete;

		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, F &&f, Args &&...args);

		const R & resultAt(const std::size_t i) const;

	private:
		Container<R>  _results;
	};



	template < class TaskPackTraits >
	class TaskPack<void, TaskPackTraits> : public internal::TaskPackBase, public TaskPackTraits {

		static_assert(std::is_void<decltype(std::declval<TaskPackTraits>().signalTaskComplete(std::declval<std::size_t>()))>::value, "TaskPackTraits template parameter must have a 'void signalTaskComplete(const std::size_t)' member function.");

	public:
		template < class ...Args >
		inline TaskPack(const std::size_t size, const Args &...args);

		template < class ...Args >
		inline TaskPack(const std::size_t size, Args &&...args);

		TaskPack(const TaskPack &) = delete;
		TaskPack(TaskPack &&) = delete;

		TaskPack & operator=(const TaskPack &) = delete;
		TaskPack & operator=(TaskPack &&) = delete;

		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, F &&f, Args &&...args);
	};

}

#include <MPMCThreadPool/inlines/MPMCThreadPool.inl>

#endif /* MPMCThreadPool_hpp */
