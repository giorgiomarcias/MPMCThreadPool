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
			inline std::size_t nCompletedTasks() const;

			inline iterator begin();
			inline const_iterator begin() const;
			inline move_iterator moveBegin();

			inline iterator end();
			inline const_iterator end() const;
			inline move_iterator moveEnd();

		protected:
			std::atomic_size_t   _nCompletedTasks;
			SimpleTaskContainer  _tasks;
		};
	}

	template < class R >
	class TaskPack : public internal::TaskPackBase {
	protected:
		using internal::TaskPackBase::Container;

	public:
		inline TaskPack(const std::size_t size);

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

	template <>
	class TaskPack<void> : public internal::TaskPackBase {
	public:
		inline TaskPack(const std::size_t size);

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
