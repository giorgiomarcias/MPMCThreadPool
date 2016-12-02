# MPMCThreadPool

C++11, Lock-Free, Multiple-Producer-Multiple-Consumer Thread Pool



## Design

The `MPMCThreadPool` class manages the lifetime of some threads.
The number of threads can be changed at runtime.
Each thread keeps dequeuing tasks to perform from a queue or it waits until any task becomes available.
It is based on the ultra-fast, lock-free, MPMC [ConcurrentQueue](https://github.com/cameron314/concurrentqueue).

The `MPMCThreadPool` class is almost entirely **lock-free**.
*Almost entirely* means that its methods are lock-free, as long as there are tasks to process.
Even methods for pushing tasks are lock-free.
Also resizing methods are lock-free.
The only part that is blocking is the work of the threads: as long as the queue is not empty they keep dequeuing in a lock-free manner; when they see that the queue is empty then they block into a condition variable and are woken up as soon as at least a task is enqueued.
The blocking part is designed by choice for avoiding wasting resources, for example in interactive applications where most of the time they are waiting for user commands.
In this case, having a number of threads running non-stop doing nothing would drain battery in vain.

Being lock-free, one can opt for an auto-balancing strategy when submitting tasks.
It is also possible to submit multiple tasks at once, for better performance, thanks to [ConcurrentQueue](https://github.com/cameron314/concurrentqueue) features.
It is possible to submit tasks to the pool from any thread.

In order to simplify the coder's life, there is a `TaskPack` template class for collecting the tasks to submit and managing the synchronization.
It takes a `TaskPackTraits` template parameter which actually provides the means of synchronization.
There three types of trait classes provided that one can use:
- `TaskPackTraitsLockFree` is completely lock-free and waits the end of the computation by running a loop on a counter.
    This is better suited for few, very short tasks.
- `TaskPackTraitsBlockingWait` is mostly lock-free.
    It can add a task to the pack which counts the completed tasks and unblocks any thread waiting for the completion to come.
    Since the counter is lock-free and always running, this class is better suited for short tasks, even being a high number.
- `TaskPackTraitsBlocking` provides a task which waits for the other tasks to complete, one by one, so not wasting CPU cycles.
    When all the tasks are complete, it wakes up any waiting thread.
    This is better suited for any number of mid-to-long tasks.
    It is actually the **default** traits.



## API

The main class is `MPMCThreadPool`, which provides the following methods:
```c++
// resizing:
void expand(n);         // add n threads
void shrink(n);         // remove n threads
// submitting tasks:
ProducerToken newProducerToken();       // create a new producer token
void submitTask(task);                  // submit a single task
void submitTask(token, task);           // submit a single task, specifying the producer token
void submitTasks(first, last);          // submit a number of tasks, from first to last (except)
void submitTasks(token, first, last);   // submit a number of tasks, from first to last (except), specifying the producer token
```
The `ProducerToken` allows the queue to optimize the submission of tasks.
See [ConcurrentQueue](https://github.com/cameron314/concurrentqueue) for more information.

This library is header-only, one-file-contained.
The interface is fully documented, just take a look at it in the code for more information.



## License

The code is subject to the simplified BSD license.
