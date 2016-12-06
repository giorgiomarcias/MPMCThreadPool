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
	std::cout << "Blocking wait traits:" << std::endl;
	mpmc_tp::TaskPack<std::size_t, mpmc_tp::TaskPackTraitsBlockingWait> taskPack1(101, std::chrono::milliseconds(10));
	for (std::size_t i = 0; i < taskPack1.size() - 1; ++i)
		taskPack1.setTaskAt(i, sum_to, i * 1000000);
	taskPack1.setWaitTaskAt(taskPack1.size() - 1);
	taskPack1.setCallback([&flag](const std::size_t i){
		while (flag.test_and_set())
			;
		std::cout << "Done task " << i << std::endl;
		flag.clear();
	});
	threadPool.submitTasks(producerToken, taskPack1.moveBegin(), taskPack1.moveEnd());
	taskPack1.wait();
	for (std::size_t i = 0; i < taskPack1.size() - 1; ++i)
		std::cout << "Result at " << i << " : " << taskPack1.resultAt(i) << std::endl;



	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "Blocking traits:" << std::endl;
	mpmc_tp::TaskPack<void, mpmc_tp::TaskPackTraitsBlockingWait> taskPack2(101, std::chrono::milliseconds(10));
	for (std::size_t i = 0; i < taskPack2.size() - 1; ++i)
		taskPack2.setTaskAt(i, count_to, i * 1000000);
	taskPack2.setWaitTaskAt(taskPack2.size() - 1);
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

	std::cout << "End" << std::endl;
}
