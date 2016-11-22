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

	std::this_thread::sleep_for(std::chrono::seconds(30));

	std::cout << "Stopping 2 threads...";
	std::cout.flush();
	threadPool.stop();
	std::cout << "Stopped!" << std::endl;
}
