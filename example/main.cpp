// Copyright (c) 2016 Giorgio Marcias
//
// This source code is
//
// Author: Giorgio Marcias
// email: marcias.giorgio@gmail.com

#include <MPMCThreadPool/MPMCThreadPool.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
	mpmc_tp::MPMCThreadPool<2> threadPool;

	std::cout << "Starting 2 threads...";
	std::cout.flush();
	threadPool.start();
	std::cout << "started!" << std::endl;

	mpmc_tp::ProducerToken producerToken = threadPool.newProducerToken();

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
	for (std::size_t i = 0; i < 10; ++i)
		threadPool.pushWork(producerToken, [&flag, i](){
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
		threadPool.pushWork(producerToken, [&flag, i](){
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

	while (flag.test_and_set())
		;
	std::cout << "Stopping 2 threads..." << std::endl;
	flag.clear();
	threadPool.stop();
	std::cout << "Stopped!" << std::endl;
}
