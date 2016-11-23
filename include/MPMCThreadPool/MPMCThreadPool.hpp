// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#ifndef MPMCThreadPool_hpp
#define MPMCThreadPool_hpp

#include <concurrentqueue/concurrentqueue.h>

namespace mpmc_tp {

	using namespace moodycamel;



	/// The MPMCThreadPool class
	template < std::size_t SIZE >
	class MPMCThreadPool {
	public:

		static_assert(SIZE > 0UL, "Invalid thread pool size: it must own at least one thread.");

		////////////////////////////////////////////////////////////////////////
		// DEFINITIONS
		////////////////////////////////////////////////////////////////////////

		static constexpr std::size_t COMPILETIME_SIZE = SIZE;

		using SimpleTaskType = std::function<void()>;

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

		inline void start();

		inline void interrupt();

		inline void stop();

		inline ProducerToken newProducerToken();

		inline void pushWork(const SimpleTaskType &task);

		inline void pushWork(SimpleTaskType &&task);

		inline void pushWork(const ProducerToken &token, const SimpleTaskType &task);

		inline void pushWork(const ProducerToken &token, SimpleTaskType &&task);

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

}

#include <MPMCThreadPool/inlines/MPMCThreadPool.inl>

#endif /* MPMCThreadPool_hpp */
