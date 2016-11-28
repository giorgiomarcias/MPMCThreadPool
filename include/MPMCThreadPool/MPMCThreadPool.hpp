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



	/// The MPMCThreadPool class represents a pool of a fixed number (at compile
	/// time) of threads.
	/// Threads are instantiated and invoked when a MPMCThreadPool object is
	/// constructed and they are kept alive for the whole lifetime of the thread
	/// pool.
	/// At any time, any instantiated thread either is performing a task or is
	/// wating for a new task to perform to become available.
	/// Users of the thread pool can post tasks and, possibly, wait for their
	/// completion.
	/// The thread pool keeps a lock-free queue of tasks (that can be copied or
	/// moved) allowing for fast single post or bulk post (which is faster than
	/// multiple single enqueuings). For further information on such performant
	/// lock-free queue see https://github.com/cameron314/concurrentqueue
	/// This thread pool is completely thread-safe and multiple producers can
	/// post tasks independently without needing to sinchronize.
	/// For slightly better performance, each producer (i.e. a user that posts
	/// tasks and runs in a given thread) can get and specify a token allowing
	/// faster enqueuing.
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
		 *    @brief Default constructor. It instantiates and invokes threads.
		 */
		inline MPMCThreadPool();


		/**
		 *    @brief Copy constructor. MPMCThreadPools can't be copied.
		 */
		MPMCThreadPool(const MPMCThreadPool &other) = delete;

		/**
		 *    @brief Move constructor. MPMCThreadPools can't be moved.
		 */
		MPMCThreadPool(MPMCThreadPool &&other) = default;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// DESTRUCTOR
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Default destructor. It stops and delete threads.
		 */
		inline ~MPMCThreadPool();

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Copy assignment operator. MPMCThreadPools can't be copied.
		 */
		MPMCThreadPool & operator=(const MPMCThreadPool &other) = delete;

		/**
		 *    @brief Move assignment operator. MPMCThreadPools can't be moved.
		 */
		MPMCThreadPool & operator=(MPMCThreadPool &&other) = default;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ACCESS METHODS
		////////////////////////////////////////////////////////////////////////


		/**
		 *    @brief Returns the size (as defined at compile time) of the pool.
		 */
		inline constexpr std::size_t size() const;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// METHODS FOR TASKS
		////////////////////////////////////////////////////////////////////////


		/**
		 *   @brief Obtain a new producer token for posting tasks faster.
		 */
		inline ProducerToken newProducerToken();


		/**
		 *   @brief Post a single task by copying it into the queue.
		 *   @param task         The task to copy into the queue.
		 */
		inline void postTask(const SimpleTaskType &task);

		/**
		 *   @brief Post a single task by moving it into the queue.
		 *   @param task         The task to move into the queue.
		 */
		inline void postTask(SimpleTaskType &&task);

		/**
		 *   @brief Post a single task by copying it into the queue, specifying
		 *          the producer token. This results in faster enqueuing.
		 *   @param token     The producer token for faster enqueuing.
		 *   @param task      The task to copy into the queue.
		 */
		inline void postTask(const ProducerToken &token, const SimpleTaskType &task);

		/**
		 *   @brief Post a single task by moving it into the queue, specifying
		 *          the producer token. This results in faster enqueuing.
		 *   @param token     The producer token for faster enqueuing.
		 *   @param task      The task to move into the queue.
		 */
		inline void postTask(const ProducerToken &token, SimpleTaskType &&task);

		/**
		 *   @brief Post a bulk of tasks. Pass a std::move_iterator for moving
		 *          tasks into the queue.
		 *   @param first     The iterator to the first task to enqueue.
		 *   @param task      The iterator to the last task (except) to enqueue.
		 */
		template < class It >
		inline void postTasks(It first, It last);

		/**
		 *   @brief Post a bulk of tasks, specifying the producer token. This
		 *          results in faster enqueuing. Pass a std::move_iterator for
		 *          moving tasks into the queue.
		 *   @param token     The producer token for faster enqueuing.
		 *   @param first     The iterator to the first task to enqueue.
		 *   @param task      The iterator to the last task (except) to enqueue.
		 */
		template < class It >
		inline void postTasks(const ProducerToken &token, It first, It last);

		////////////////////////////////////////////////////////////////////////


	private:
		////////////////////////////////////////////////////////////////////////
		// PRIVATE DEFINITIONS
		////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// PRIVATE METHODS
		////////////////////////////////////////////////////////////////////////


		/**
		 *   @brief This specifies the job of the threads: they wait for tasks
		 *          to be enqueued, dequeue one of them and perform it. They
		 *          loop in this wait-dequeue-perform until the thread pool is
		 *          destructed.
		 */
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

		class TaskPackTraitsBase {
		public:
			inline TaskPackTraitsBase(const std::size_t size);

			template < class Rep, class Period >
			inline TaskPackTraitsBase(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

			template < class Rep, class Period >
			inline TaskPackTraitsBase(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

			inline TaskPackTraitsBase(const TaskPackTraitsBase &) = delete;
			inline TaskPackTraitsBase(TaskPackTraitsBase &&) = delete;

			inline TaskPackTraitsBase & operator=(const TaskPackTraitsBase &) = delete;
			inline TaskPackTraitsBase & operator=(TaskPackTraitsBase &&) = delete;

			template < class Rep, class Period >
			inline void setInterval(const std::chrono::duration<Rep, Period> &interval);

			template < class C, class ...Args >
			inline void setCallback(const C &c, const Args &...args);

			template < class C, class ...Args >
			inline void setCallback(C &&c, Args &&...args);

			inline void signalTaskComplete(const std::size_t i);

			inline std::size_t nCompletedTasks() const;

			inline void wait() const;

		protected:
			std::size_t                      _size;
			std::atomic_size_t               _nCompletedTasks;
			std::chrono::nanoseconds         _interval;
			std::function<void(std::size_t)> _callback;
			std::atomic_bool                 _completed;
			mutable std::mutex               _mutex;
			mutable std::condition_variable  _condVar;
		};

	}




	template < class R >
	class TaskPackTraitsSimple : public internal::TaskPackTraitsBase {
	public:
		inline TaskPackTraitsSimple(const std::size_t size);

		template < class Rep, class Period >
		inline TaskPackTraitsSimple(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

		template < class Rep, class Period >
		inline TaskPackTraitsSimple(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

		inline TaskPackTraitsSimple(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple(TaskPackTraitsSimple &&) = delete;

		inline TaskPackTraitsSimple & operator=(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple & operator=(TaskPackTraitsSimple &&) = delete;

		template < class F, class ...Args >
		inline void setReduce(F &&f, Args &&...args);

		inline const R & waitCompletionAndReduce();

		inline std::function<R()> createWaitTask();

		inline const R & getResult() const;

	private:
		std::function<R()>               _reduce;
		R                                _reducedResult;
	};



	template <>
	class TaskPackTraitsSimple<void> : public internal::TaskPackTraitsBase {
	public:
		inline TaskPackTraitsSimple(const std::size_t size);

		template < class Rep, class Period >
		inline TaskPackTraitsSimple(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

		template < class Rep, class Period >
		inline TaskPackTraitsSimple(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

		inline TaskPackTraitsSimple(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple(TaskPackTraitsSimple &&) = delete;

		inline TaskPackTraitsSimple & operator=(const TaskPackTraitsSimple &) = delete;
		inline TaskPackTraitsSimple & operator=(TaskPackTraitsSimple &&) = delete;

		inline void waitCompletion();

		inline std::function<void()> createWaitTask();
	};



	using TaskPackTraitsDefault = TaskPackTraitsSimple<void>;



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
