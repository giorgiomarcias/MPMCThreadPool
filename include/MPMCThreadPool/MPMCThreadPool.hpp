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

namespace mpmc_tp {

	using namespace moodycamel;



	////////////////////////////////////////////////////////////////////////////
	// NAMESPACE-LEVEL DEFINITIONS
	////////////////////////////////////////////////////////////////////////////

	using SimpleTaskType = std::function<void()>;

	////////////////////////////////////////////////////////////////////////////



	/// The MPMCThreadPool class represents a pool of a fixed number of threads.
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
	class MPMCThreadPool {
	public:

		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *    @brief Default constructor. It instantiates and invokes threads.
		 */
		inline MPMCThreadPool(const std::size_t size);

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
		 *    @brief Returns the size of the pool.
		 */
		inline std::size_t size() const;

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

		std::vector<std::thread>         _threads;  ///< Array of thread objects.

		ConcurrentQueue<SimpleTaskType>  _taskQueue;///< Queue of tasks.

		std::atomic_bool                 _active;   ///< Signal for stopping the threads.

		std::mutex                       _mutex;    ///< Mutex for allowing thread suspension when the queue is empty.
		std::condition_variable          _condVar;  ///< Condition variable for thread wakeup when the queue is no more empty.

		////////////////////////////////////////////////////////////////////////

	};






	////////////////////////////////////////////////////////////////////////////
	// TRAITS
	////////////////////////////////////////////////////////////////////////////

	/// The TaskPackTraitsLockFree class is a base class for any TaskPack traits
	/// providing mandatory as well as recommended methods. The only
	/// mandatory method is 'signalTaskComplete' which is called whenever a
	/// task has been completed. It is possible to set a callback compound
	/// to this. Furthermore, a 'wait' method can be used to block the
	/// calling thread until all tasks have been completed.
	/// This class is lock-free, using an atomic counter to keep information
	/// updated on the number of complete tasks.
	/// This traits are most suitable for a pack with few short tasks.
	class TaskPackTraitsLockFree {
	public:
		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size The size corresponds to the number of packed tasks.
		 */
		inline TaskPackTraitsLockFree(const std::size_t size);

		/**
		 *   @brief Constructor with initial size and check interval.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (copied).
		 */
		template < class Rep, class Period >
		inline TaskPackTraitsLockFree(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

		/**
		 *   @brief Constructor with initial size and check interval.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (moved).
		 */
		template < class Rep, class Period >
		inline TaskPackTraitsLockFree(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

		/**
		 *   @brief Copy constructor deleted.
		 */
		inline TaskPackTraitsLockFree(const TaskPackTraitsLockFree &) = delete;

		/**
		 *   @brief Move constructor deleted.
		 */
		inline TaskPackTraitsLockFree(TaskPackTraitsLockFree &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// DESTRUCTOR
		////////////////////////////////////////////////////////////////////////

		virtual inline ~TaskPackTraitsLockFree() = default;

		////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Copy assignment operator deleted.
		 */
		inline TaskPackTraitsLockFree & operator=(const TaskPackTraitsLockFree &) = delete;

		/**
		 *   @brief Move assignment operator deleted.
		 */
		inline TaskPackTraitsLockFree & operator=(TaskPackTraitsLockFree &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// MAIN METHODS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Set the interval between a check for completion and next one.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (copied).
		 */
		template < class Rep, class Period >
		inline void setInterval(const std::chrono::duration<Rep, Period> &interval);

		/**
		 *   @brief Set the interval between a check for completion and next one.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (moved).
		 */
		template < class Rep, class Period >
		inline void setInterval(std::chrono::duration<Rep, Period> &&interval);

		/**
		 *   @brief Set function to call at every task complete signal. (copy)
		 *   @param c        The callback function has form 'void c(std::size_t
		 *                   i, Args ...args)' where 'i' (mandatory) is the
		 *                   index of the just completed task, and 'args' are
		 *                   possibly other parameters to bind to the callback.
		 *   @param args     Possible paramenter arguments for the callback.
		 */
		template < class C, class ...Args >
		inline void setCallback(const C &c, const Args &...args);

		/**
		 *   @brief Set function to call at every task complete signal. (move)
		 *   @param c        The callback function has form 'void c(std::size_t
		 *                   i, Args ...args)' where 'i' (mandatory) is the
		 *                   index of the just completed task, and 'args' are
		 *                   possibly other parameters to bind to the callback.
		 *   @param args     Possible paramenter arguments for the callback.
		 */
		template < class C, class ...Args >
		inline void setCallback(C &&c, Args &&...args);


		/**
		 *   @brief The signal indicating the i-th task has been completed.
		 *          Mandatory.
		 *   @param i        The index of the task just completed.
		 *   @note If a callback has bee provided, it gets called here.
		 */
		virtual inline void signalTaskComplete(const std::size_t i);


		/**
		 *   @brief Return the number of completed tasks so far.
		 */
		virtual inline std::size_t nCompletedTasks() const;


		/**
		 *   @brief Wait for the packed tasks to complete. It is lock-free,
		 *          relying on a loop, so it is better to use this traits for 
		 *          few, short tasks. Call this from the task producer.
		 */
		virtual inline void wait() const;

		////////////////////////////////////////////////////////////////////////

	protected:
		/**
		 *   @brief Wait for the packed tasks to complete. It is lock-free,
		 *          relying on a loop, so it is better to use this traits for
		 *          few, short tasks. Call this from the task producer.
		 */
		virtual inline void waitComplete() const;

		std::size_t                      _size;            ///< The number of packed tasks.
		std::atomic_size_t               _nCompletedTasks; ///< The number of completed tasks so far.
		std::chrono::nanoseconds         _interval;        ///< The time to wait between a check and the next in wait().
		std::function<void(std::size_t)> _callback;        ///< Optional callback to call inside signalTaskComplete().
	};



	/// The TaskPackTraitsBlockingWait class is a base class for any
	/// TaskPack traits, similarly to TaskPackTraitsLockFree.
	/// This class adds a blocking 'wait' method.
	/// This class is mostly lock-free, using an atomic counter to keep
	/// information updated on the number of complete tasks, like
	/// TaskPackTraitsLockFree. The only blocking part is, of course, the 'wait'
	/// method. It relies on a mutex and a condition variable, as well as an
	/// atomic bool. Deriving classes, whose users want to block themselves
	/// calling 'wait', should atomically set (in 'release' order) the
	/// '_completed' bool variable and notify blocked threads with the
	/// '_waitCondVar' condition variable.
	/// This traits are most suitable for a pack with many short tasks.
	class TaskPackTraitsBlockingWait : public TaskPackTraitsLockFree {
	public:
		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks.
		 */
		inline TaskPackTraitsBlockingWait(const std::size_t size);

		/**
		 *   @brief Constructor with initial size and check interval.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (copied).
		 */
		template < class Rep, class Period >
		inline TaskPackTraitsBlockingWait(const std::size_t size, const std::chrono::duration<Rep, Period> &interval);

		/**
		 *   @brief Constructor with initial size and check interval.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param interval The amount of time to wait between a check and the
		 *                   next one while wating for completion (moved).
		 */
		template < class Rep, class Period >
		inline TaskPackTraitsBlockingWait(const std::size_t size, std::chrono::duration<Rep, Period> &&interval);

		/**
		 *   @brief Copy constructor deleted.
		 */
		inline TaskPackTraitsBlockingWait(const TaskPackTraitsBlockingWait &) = delete;

		/**
		 *   @brief Move constructor deleted.
		 */
		inline TaskPackTraitsBlockingWait(TaskPackTraitsBlockingWait &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Copy assignment operator deleted.
		 */
		inline TaskPackTraitsBlockingWait & operator=(const TaskPackTraitsBlockingWait &) = delete;

		/**
		 *   @brief Move assignment operator deleted.
		 */
		inline TaskPackTraitsBlockingWait & operator=(TaskPackTraitsBlockingWait &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// MAIN METHODS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Wait for the packed tasks to complete. It is blocking,
		 *          relying on a mutex and a condition variable. Derived classes
		 *          should provide a way to wake up waiting threads by setting
		 *          '_completed' to true and notifying all waiting threads with
		 *          '_waitCondVar'. It is better for a pack with many short
		 *          tasks. Call this from the task producer.
		 */
		inline void wait() const override;

		////////////////////////////////////////////////////////////////////////

	protected:
		/**
		 *   @brief Wait for the packed tasks to complete. It is lock-free,
		 *          relying on a loop, so it is better to use this traits for
		 *          few, short tasks. Call this from the task producer.
		 */
		inline void waitComplete() const override;

		mutable std::atomic_bool         _completed;   ///< Flag indicating whether all the tasks have been completed.
		mutable std::mutex               _waitMutex;   ///< Mutex for blocking the waiting threads.
		mutable std::condition_variable  _waitCondVar; ///< Condition variable for blocking/waking up waiting threads.
	};



	/// The TaskPackTraitsBlocking class is a base class for any TaskPack
	/// traits, similarly to TaskPackTraitsLockFree.
	/// This class adds a blocking 'signalCompletedTask' and 'wait' methods.
	/// This class uses an atomic counter to keep information updated on the
	/// number of complete tasks, like TaskPackTraitsLockFree.
	/// The only blocking part is, of course, the 'wait' and the
	/// 'signalCompleteTask' methods. They rely on mutexes and condition
	/// variables, as well as atomic booleans. Deriving classes, whose users
	/// want to block themselves calling 'wait', should atomically set (in
	///  'release' order) the '_completed' bool variable and notify blocked
	/// threads with the '_waitCondVar' condition variable. There is not need of
	/// any interval here, since the 'wait' method blocks until a new task has
	/// been completed (and notified).
	/// This traits are most suitable for a pack with many short tasks.
	class TaskPackTraitsBlocking : public TaskPackTraitsBlockingWait {
	public:
		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks.
		 */
		inline TaskPackTraitsBlocking(const std::size_t size);

		/**
		 *   @brief Copy constructor deleted.
		 */
		inline TaskPackTraitsBlocking(const TaskPackTraitsBlocking &) = delete;

		/**
		 *   @brief Move constructor deleted.
		 */
		inline TaskPackTraitsBlocking(TaskPackTraitsBlocking &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Copy assignment operator deleted.
		 */
		inline TaskPackTraitsBlocking & operator=(const TaskPackTraitsBlocking &) = delete;

		/**
		 *   @brief Move assignment operator deleted.
		 */
		inline TaskPackTraitsBlocking & operator=(TaskPackTraitsBlocking &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// MAIN METHODS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief The signal indicating the i-th task has been completed.
		 *          Mandatory.
		 *   @param i        The index of the task just completed.
		 *   @note If a callback has bee provided, it gets called here.
		 */
		inline void signalTaskComplete(const std::size_t i) override;

		////////////////////////////////////////////////////////////////////////

	protected:
		/**
		 *   @brief Wait for the packed tasks to complete. It is lock-free,
		 *          relying on a loop, so it is better to use this traits for
		 *          few, short tasks. Call this from the task producer.
		 */
		inline void waitComplete() const override;

		mutable std::mutex               _signalMutex;   ///< Mutex for blocking the wait method.
		mutable std::condition_variable  _signalCondVar; ///< Condition variable for blocking/waking up wait method.
	};



	/// Default traits.
	using TaskPackTraitsDefault = TaskPackTraitsBlocking;

	////////////////////////////////////////////////////////////////////////////






	////////////////////////////////////////////////////////////////////////////
	// INTERNAL STUFF
	////////////////////////////////////////////////////////////////////////////

	namespace internal {

		/// The TaskPackBase class exposes the common methods for a TaskPack
		/// object. It owns a container of SimpleTaskType tasks and gives some
		/// begin/end methods to access them: use these to bulk enqueue the pack
		/// into the thread pool.
		class TaskPackBase {
		protected:
			template < class T >
			using Container = std::vector<T>;
			using SimpleTaskContainer = Container<SimpleTaskType>;

		public:
			using iterator       = SimpleTaskContainer::iterator;
			using const_iterator = SimpleTaskContainer::const_iterator;
			using move_iterator  = std::move_iterator<iterator>;


			////////////////////////////////////////////////////////////////////
			// CONSTRUCTORS
			////////////////////////////////////////////////////////////////////

			/**
			 *   @brief Constructor with initial size.
			 *   @param size     The size corresponds to the number of packed
			 *                   tasks.
			 */
			inline TaskPackBase(const std::size_t size);

			/**
			 *   @brief Copy constructor deleted.
			 */
			TaskPackBase(const TaskPackBase &) = delete;

			/**
			 *   @brief Move constructor deleted.
			 */
			TaskPackBase(TaskPackBase &&) = delete;

			////////////////////////////////////////////////////////////////////



			////////////////////////////////////////////////////////////////////
			// ASSIGNMENT OPERATORS
			////////////////////////////////////////////////////////////////////

			/**
			 *   @brief Copy assignment operator deleted.
			 */
			TaskPackBase & operator=(const TaskPackBase &) = delete;

			/**
			 *   @brief Move assignment operator deleted.
			 */
			TaskPackBase & operator=(TaskPackBase &&) = delete;

			////////////////////////////////////////////////////////////////////



			////////////////////////////////////////////////////////////////////
			// MAIN METHODS
			////////////////////////////////////////////////////////////////////

			/**
			 *    @brief Returns the size of the task pack.
			 */
			inline std::size_t size() const;

			/**
			 *    @brief Returns an iterator to the beginning.
			 */
			inline iterator begin();

			/**
			 *    @brief Returns a constant iterator to the beginning.
			 */
			inline const_iterator begin() const;

			/**
			 *    @brief Returns a move iterator to the beginning.
			 */
			inline move_iterator moveBegin();

			/**
			 *    @brief Returns an iterator to the end.
			 */
			inline iterator end();

			/**
			 *    @brief Returns a constant iterator to the end.
			 */
			inline const_iterator end() const;

			/**
			 *    @brief Returns a move iterator to the end.
			 */
			inline move_iterator moveEnd();


			/**
			 *   @brief Give const access to the i-th task.
			 *   @param i    The index of the task to return.
			 *   @return The i-th task.
			 */
			inline const SimpleTaskType & at(const std::size_t i) const;

			/**
			 *   @brief Give access to the i-th task.
			 *   @param i    The index of the task to return.
			 *   @return The i-th task.
			 */
			inline SimpleTaskType & at(const std::size_t i);

			/**
			 *   @brief Give const access to the i-th task.
			 *   @param i    The index of the task to return.
			 *   @return The i-th task.
			 */
			inline const SimpleTaskType & operator[](const std::size_t i) const;

			/**
			 *   @brief Give access to the i-th task.
			 *   @param i    The index of the task to return.
			 *   @return The i-th task.
			 */
			inline SimpleTaskType & operator[](const std::size_t i);

			////////////////////////////////////////////////////////////////////

		protected:
			SimpleTaskContainer  _tasks;  ///< Container of SimpleTaskType tasks.
		};

	}

	////////////////////////////////////////////////////////////////////////////



	/// The TaskPack class is the most general class for packing tasks.
	/// It needs two template parameters:
	/// @param R               is the return type of the tasks to perform
	/// @param TaskPackTraits  is a class for handling signals and the wait task
	///                        whenever a task has been completed and when all
	///                        tasks have been completed, respectively.
	/// The TaskPackTraits template parameter must provide at least:
	/// - a constructor taking at least a std::size_t as first parameter,
	/// - a 'void signalTaskComplete(std::size_t)' method for signaling a task
	///     at position i has been completed, and
	/// - a 'void waitComplete()' method for waiting to the end of the tasks.
	/// NOTE: the size of the task container is always one plus the size
	/// given as parameter to the constructors and one plus the size given to
	/// the traits constructor.
	template < class R, class TaskPackTraits = TaskPackTraitsDefault >
	class TaskPack : public internal::TaskPackBase, public TaskPackTraits {
	protected:
		using internal::TaskPackBase::Container;

	public:
		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks..
		 *   @param args     Other possible parameters the traits constructor
		 *                   may need. (copy)
		 */
		template < class ...Args >
		inline TaskPack(const std::size_t size, const Args &...args);

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param args     Other possible parameters the traits constructor
		 *                   may need. (move)
		 */
		template < class ...Args >
		inline TaskPack(const std::size_t size, Args &&...args);

		/**
		 *   @brief Copy constructor deleted.
		 */
		TaskPack(const TaskPack &) = delete;

		/**
		 *   @brief Move constructor deleted.
		 */
		TaskPack(TaskPack &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Copy assignment operator deleted.
		 */
		TaskPack & operator=(const TaskPack &) = delete;

		/**
		 *   @brief Move assignment operator deleted.
		 */
		TaskPack & operator=(TaskPack &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// MAIN METHODS
		////////////////////////////////////////////////////////////////////////


		/**
		 *   @brief Set a function as a task at position i.
		 *   @param i        Index to the container where to store the task.
		 *   @param f        The function to set as task. (copy)
		 *   @param args     Possible parameters to bind to f. (copy)
		 */
		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, const F &f, const Args &...args);

		/**
		 *   @brief Set a function as a task at position i.
		 *   @param i        Index to the container where to store the task.
		 *   @param f        The function to set as task. (move)
		 *   @param args     Possible parameters to bind to f. (move)
		 */
		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, F &&f, Args &&...args);

		/**
		 *   @brief Set the wait task at position i. It can be called at most
		 *          once, otherwise a std::logic_error is thrown.
		 *   @param i        The index of the container where to store the wait
		 *                   task.
		 */
		inline void setWaitTaskAt(const std::size_t i);


		/**
		 *   @brief Get the result of the task at position i.
		 *   @param i        Index of the task result to access.
		 *   @return The result of the task at position i.
		 *   @note It is not thread-safe but it is guaranteed to hold the value
		 *         when a signal for the corresponding task is emitted.
		 */
		inline const R & resultAt(const std::size_t i) const;

		////////////////////////////////////////////////////////////////////////

	private:
		Container<R>  _results;
	};



	template < class TaskPackTraits >
	class TaskPack<void, TaskPackTraits> : public internal::TaskPackBase, public TaskPackTraits {
	public:
		////////////////////////////////////////////////////////////////////////
		// CONSTRUCTORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param args     Other possible parameters the traits constructor
		 *                   may need. (copy)
		 */
		template < class ...Args >
		inline TaskPack(const std::size_t size, const Args &...args);

		/**
		 *   @brief Constructor with initial size. The default interval is 0.
		 *   @param size     The size corresponds to the number of packed tasks.
		 *   @param args     Other possible parameters the traits constructor
		 *                   may need. (move)
		 */
		template < class ...Args >
		inline TaskPack(const std::size_t size, Args &&...args);

		/**
		 *   @brief Copy constructor deleted.
		 */
		TaskPack(const TaskPack &) = delete;

		/**
		 *   @brief Move constructor deleted.
		 */
		TaskPack(TaskPack &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// ASSIGNMENT OPERATORS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Copy assignment operator deleted.
		 */
		TaskPack & operator=(const TaskPack &) = delete;

		/**
		 *   @brief Move assignment operator deleted.
		 */
		TaskPack & operator=(TaskPack &&) = delete;

		////////////////////////////////////////////////////////////////////////



		////////////////////////////////////////////////////////////////////////
		// MAIN METHODS
		////////////////////////////////////////////////////////////////////////

		/**
		 *   @brief Set a function as a task at position i.
		 *   @param i        Index to the container where to store the task.
		 *   @param f        The function to set as task. (copy)
		 *   @param args     Possible parameters to bind to f. (copy)
		 */
		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, const F &f, const Args &...args);

		/**
		 *   @brief Set a function as a task at position i.
		 *   @param i        Index to the container where to store the task.
		 *   @param f        The function to set as task. (move)
		 *   @param args     Possible parameters to bind to f. (move)
		 */
		template < class F, class ...Args >
		inline void setTaskAt(const std::size_t i, F &&f, Args &&...args);

		/**
		 *   @brief Set the wait task at position i. It can be called at most
		 *          once, otherwise a std::logic_error is thrown.
		 *   @param i        The index of the container where to store the wait
		 *                   task.
		 */
		inline void setWaitTaskAt(const std::size_t i);

		////////////////////////////////////////////////////////////////////////

	};

}

#include <MPMCThreadPool/inlines/MPMCThreadPool.inl>

#endif /* MPMCThreadPool_hpp */
