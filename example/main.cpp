// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
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

	std::cout << "Starting 2 threads...";
	std::cout.flush();
	mpmc_tp::MPMCThreadPool<2> threadPool;
	std::cout << "started!" << std::endl;

	mpmc_tp::ProducerToken producerToken = threadPool.newProducerToken();

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
	for (std::size_t i = 0; i < 10; ++i)
		threadPool.pushTask(producerToken, [&flag, i](){
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

	for (std::size_t i = 10; i < 20; ++i)
		threadPool.pushTask(producerToken, [&flag, i](){
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



	mpmc_tp::TaskPack<std::size_t, mpmc_tp::TaskPackTraitsSimpleBlocking<std::size_t>> taskPack0(101, std::chrono::milliseconds(10));
	for (std::size_t i = 0; i < taskPack0.size()-1; ++i)
		taskPack0.setTaskAt(i, sum_to, i * 1000000);
//	threadPool.pushTask(producerToken, [&flag, &taskPack0](){
//		std::stringstream stream;
//		stream << "Completed " << std::setw(3) << std::setfill(' ') << taskPack0.nCompletedTasks();
//		while (flag.test_and_set())
//			;
//		std::cout << stream.str();
//		std::cout.flush();
//		flag.clear();
//		std::size_t nChars = stream.str().size();
//		while (taskPack0.nCompletedTasks() < taskPack0.size()) {
//			stream.str(std::string());
//			stream << "Completed " << std::setw(3) << std::setfill(' ') << taskPack0.nCompletedTasks();
//			while (flag.test_and_set())
//				;
//			for (std::size_t c = 0; c < nChars; ++c)
//				std::cout << '\b';
//			std::cout << stream.str();
//			std::cout.flush();
//			flag.clear();
//			nChars = stream.str().size();
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		}
//		stream.str(std::string());
//		stream << "Completed " << std::setw(3) << std::setfill(' ') << taskPack0.nCompletedTasks();
//		while (flag.test_and_set())
//			;
//		for (std::size_t c = 0; c < nChars; ++c)
//			std::cout << '\b';
//		std::cout << stream.str() << std::endl;
//		flag.clear();
//	});
	taskPack0.setReduce([&taskPack0]()->std::size_t{
		std::size_t total = 0;
		for (std::size_t i = 0; i < taskPack0.size(); ++i)
			total += taskPack0.resultAt(i);
		return total;
	});
	taskPack0.setTaskAt(taskPack0.size()-1, taskPack0.createWaitTask());
	threadPool.pushTasks(producerToken, taskPack0.moveBegin(), taskPack0.moveEnd());
	taskPack0.wait();
	std::cout << "Result = " << taskPack0.getResult() << std::endl;
}
