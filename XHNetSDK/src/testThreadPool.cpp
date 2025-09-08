//#include <iostream>
//#include <chrono>
//#include <vector>
//#include <functional>
//
//#include "thread_pool.h"
//
//// Some example tasks for the thread pools
//void task1() {
//	std::cout << "Task 1 executed by thread " << std::this_thread::get_id() << std::endl;
//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
//}
//
//void task2(int num) {
//	std::cout << "Task 2 executed by thread " << std::this_thread::get_id() << " with argument: " << num << std::endl;
//	std::this_thread::sleep_for(std::chrono::milliseconds(20));
//}
//
//void priorityTask(int priority) {
//	std::cout << "Priority Task executed by thread " << std::this_thread::get_id() << " with priority: " << priority << std::endl;
//	std::this_thread::sleep_for(std::chrono::milliseconds(priority*1000));
//}
//void myTask(int arg1, const std::string& arg2)
//{
//
//	std::cout << "Task executed with arg1 = " << arg1 << ", arg2 = " << arg2 << std::endl;
//}
//int main() {
//	const int numThreads = 4;
//
//	// Test ThreadPool
//	netlib::ThreadPool* threadPool = netlib::ThreadPool::GetInstance();
//	threadPool->start();
//
//	std::cout << "ThreadPool is running with " << threadPool->getThreadNum() << " threads." << std::endl;
//
//	// Queue tasks
//	for (int i = 0; i < 10; ++i) {
//		threadPool->append([]() { task1(); });
//		threadPool->appendArg<void(int)>(task2, i); // Explicitly specify the template argument
//	}
//
//
//
//	int arg1 = 42;
//	std::string arg2 = "Hello, world!";
//	threadPool->appendArg(myTask, arg1, arg2);
//
//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//	// Test ThreadPriorityPool
//	netlib::ThreadPriorityPool priorityPool(numThreads);
//	priorityPool.start();
//
//	std::cout << "ThreadPriorityPool is running with " << priorityPool.getThreadNum() << " threads." << std::endl;
//
//	// Queue priority tasks
//	std::vector<int> priorities = { 1, 3, 2, 2, 1, 3, 1, 2, 3, 2 };
//	for (int priority : priorities) {
//		priorityPool.append([priority]() { priorityTask(priority); }, priority);
//	}
//
//	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//	// Stop and clean up both pools
//	threadPool->stop();
//	priorityPool.stop();
//
//	delete threadPool;
//
//	return 0;
//}
