// Copyright (c) 2016 Giorgio Marcias
//
// This source code is subject to the simplified BSD license.
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#include <MPMCThreadPool/MPMCThreadPool.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

std::size_t sum_to(const std::size_t n)
{
	std::size_t total = 0;
	for (std::size_t i = 0; i < n; ++i)
		++total;
	return total;
}

void count_to(const std::size_t n)
{
	sum_to(n);
}



int main(int argc, char *argv[])
{
	std::cout << "Starting " << mpmc_tp::MPMCThreadPool::DEFAULT_SIZE() << " threads...";
	std::cout.flush();
	mpmc_tp::MPMCThreadPool threadPool;
	std::cout << "started!" << std::endl;

	mpmc_tp::ProducerToken producerToken = threadPool.newProducerToken();

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
	for (std::size_t i = 0; i < 10; ++i)
		threadPool.submitTask(producerToken, [&flag, i](){
			while (flag.test_and_set())
				;
			std::cout << "Done task " << i << std::endl;
			flag.clear();
		});

	while (flag.test_and_set())
		;
	std::cout << "Adding 2 threads..." << std::endl;
	flag.clear();
	threadPool.expand(2);
	while (flag.test_and_set())
		;
	std::cout << "Added 2 threads: total size: " << threadPool.size() << std::endl;
	std::cout << "Sleep for 10 seconds..." << std::endl;
	flag.clear();
	std::this_thread::sleep_for(std::chrono::seconds(10));

	for (std::size_t i = 10; i < 20; ++i)
		threadPool.submitTask(producerToken, [&flag, i](){
			while (flag.test_and_set())
				;
			std::cout << "Done task " << i << std::endl;
			flag.clear();
		});

	while (flag.test_and_set())
		;
	std::cout << "Sleep for 10 seconds..." << std::endl;
	flag.clear();
	std::this_thread::sleep_for(std::chrono::seconds(10));



	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "LockFree traits:" << std::endl;
	mpmc_tp::TaskPack<std::size_t, mpmc_tp::TaskPackTraitsLockFree> taskPack0(100, std::chrono::milliseconds(10));
	for (std::size_t i = 0; i < taskPack0.size(); ++i)
		taskPack0.setTaskAt(i, sum_to, i * 1000000);
	taskPack0.setCallback([&flag](const std::size_t i){
		while (flag.test_and_set())
			;
		std::cout << "Done task " << i << std::endl;
		flag.clear();
	});
	threadPool.submitTasks(producerToken, taskPack0.moveBegin(), taskPack0.moveEnd());
	taskPack0.wait();
	for (std::size_t i = 0; i < taskPack0.size(); ++i)
		std::cout << "Result at " << i << " : " << taskPack0.resultAt(i) << std::endl;




	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "Blocking traits:" << std::endl;
	mpmc_tp::TaskPack<void, mpmc_tp::TaskPackTraitsBlocking> taskPack2(100, std::chrono::milliseconds(10));
	for (std::size_t i = 0; i < taskPack2.size(); ++i)
		taskPack2.setTaskAt(i, count_to, i * 1000000);
	taskPack2.setCallback([&flag](const std::size_t i){
		while (flag.test_and_set())
			;
		std::cout << "Done task " << i << std::endl;
		flag.clear();
	});
	threadPool.submitTasks(producerToken, taskPack2.moveBegin(), taskPack2.moveEnd());

	while (flag.test_and_set())
		;
	std::cout << "Removing 2 threads..." << std::endl;
	flag.clear();
	threadPool.shrink(2);
	while (flag.test_and_set())
		;
	std::cout << "Removed 2 threads: total size: " << threadPool.size() << std::endl;
	flag.clear();

	taskPack2.wait();



	std::cout << "Testing deadlocks:" << std::endl;
	std::default_random_engine engine(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<std::size_t> distributor(1, 1000);
	std::cout << "Lock-free wait...";
	std::cout.flush();
	for (std::size_t test = 0; test < 100; ++test) {
		mpmc_tp::TaskPack<std::size_t, mpmc_tp::TaskPackTraitsLockFree> pack(distributor(engine) + 1);
		for (std::size_t i = 0; i < pack.size(); ++i)
			pack.setTaskAt(i, sum_to, distributor(engine));
		threadPool.submitTasks(producerToken, pack.moveBegin(), pack.moveEnd());
		pack.wait();
	}
	std::cout << "done" << std::endl;
	std::cout << "Blocking...";
	std::cout.flush();
	for (std::size_t test = 0; test < 100; ++test) {
		mpmc_tp::TaskPack<std::size_t, mpmc_tp::TaskPackTraitsBlocking> pack(distributor(engine));
		for (std::size_t i = 0; i < pack.size(); ++i)
			pack.setTaskAt(i, sum_to, distributor(engine));
		threadPool.submitTasks(producerToken, pack.moveBegin(), pack.moveEnd());
		pack.wait();
	}
	std::cout << "done" << std::endl;
	std::cout << "No deadlocks." << std::endl;

	std::cout << "End" << std::endl;
}
