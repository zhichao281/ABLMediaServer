

#include "thread_pool.h"

#include <iostream>

using namespace netlib;

ThreadPool::ThreadPool(int threadNumber)
:m_nThreadNumber(threadNumber),
m_bRunning(false)
{
	start();
}

ThreadPool::~ThreadPool()
{	
	stop();
}

bool ThreadPool::start(void)
{
	if (m_bRunning.exchange(true))
	{
		return false; // Thread pool is already running
	}
	//m_vecThread.resize(m_nThreadNumber);
	for (int i = 0; i < m_nThreadNumber; i++)
	{
		m_vecThread.push_back(std::make_shared<std::thread>(std::bind(&ThreadPool::threadWork, this)));//ѭ�������߳�     
	}

	return true;

}

bool ThreadPool::stop(void)
{

	if (!m_bRunning.exchange(false))
	{
		return false; // Thread pool is not running
	}
	m_condition_empty.notify_all();
	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	for (auto& thread : m_vecThread)
	{
		if (thread)
		{
			if (thread->joinable())
			{
				thread->join();
			}
		}	
	}
	return true;
}

bool ThreadPool::IsRunning()
{
	return m_bRunning.load();
}




/*
*	Function:		append
*	Explanation:	��ӹ���������
*	Input:			ThreadTask			����

bPriority	�Ƿ�Ҫ����ִ��

*	Return:			true �ɹ� false ʧ��
*/
bool ThreadPool::append(ThreadTask task,  bool bPriority /* = false */)
{
	if (m_bRunning.load() == false)
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_taskList.emplace(task);   //������������������
	}

	m_condition_empty.notify_one();//����ĳ���߳���ִ�д�����
	return true;
}

int ThreadPool::getThreadNum() const
{
	return m_nThreadNumber;
}

int ThreadPool::getCompletedTaskCount() const
{
	return m_nCompletedTasks.load();
}

ThreadPool& netlib::ThreadPool::getInstance()
{
	auto thread_count = std::thread::hardware_concurrency(); //unsigned ��ʾ�ľ��� unsigned int

	if (thread_count > 0 && thread_count <= 4)
		thread_count = thread_count * 4 * 2;
	else if (thread_count > 4 && thread_count <= 8)
		thread_count = thread_count * 3 * 2;
	else if (thread_count > 8 && thread_count <= 32)
		thread_count = thread_count * 2 * 2;
	else
		thread_count = thread_count * 2;
	
	if (thread_count > 256)
		thread_count = MAX_THREAD;


	static ThreadPool instance(thread_count);
	
	return instance;
}

void ThreadPool::threadWork()
{
	while (m_bRunning.load())
	{
		ThreadTask task;
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			//��std::condition_variable��wait()�����У����ȴ�����������һ��������Ϊfalseʱ���̻߳ᱻ������
//����������в�Ϊ��(tasks_.empty()Ϊfalse)�����̳߳��Ѿ���ֹͣ(stop_Ϊtrue)ʱ
//���߳�Ӧ�ü���ִ����ȥ�����ȴ�����Ϊtrueʱ���̻߳��wait()�����з��أ�����ִ�к���Ĵ��롣
			m_condition_empty.wait(lock, [this]() { return !m_taskList.empty() || !m_bRunning.load(); });
			//�ȴ���������������
			if (!m_bRunning.load())
			{
				return; // Thread pool is stopping, exit thread
			}
			 if (!m_taskList.empty())
			{
				task = std::move(m_taskList.front());
				m_taskList.pop();
			}		
		}

		task();

		++m_nCompletedTasks;
	}
}

ThreadTask ThreadPool::get_one_task()
{
	//��std::condition_variable��wait()�����У����ȴ�����������һ��������Ϊfalseʱ��
	//�̻߳ᱻ������
	//����������У�����ʹ����lambda���ʽ������ȴ�������
	//����������в�Ϊ��(tasks_.empty()Ϊfalse)�����̳߳��Ѿ���ֹͣ(stop_Ϊtrue)ʱ
	//���߳�Ӧ�ü���ִ����ȥ�����ȴ�����Ϊtrueʱ���̻߳��wait()�����з��أ�����ִ�к���Ĵ��롣
	ThreadTask task = nullptr;
	std::unique_lock<std::mutex> _lock(m_mutex);
	if (m_taskList.empty())
	{
		m_condition_empty.wait(_lock);  //�ȴ���������������
	}
	if (!m_taskList.empty() && m_bRunning.load())
	{
		task = std::move(m_taskList.front());  //����������л�ȡ�ʼ����
		m_taskList.pop();     //��ȡ�ߵ����񵯳��������			
	}
	return task;

}


ThreadPriorityPool::ThreadPriorityPool(int threadNumber)
	: m_nThreadNumber(threadNumber),
	m_bRunning(false),
	m_vecThread(m_nThreadNumber)
{
}

ThreadPriorityPool::~ThreadPriorityPool()
{
	stop();
}


bool ThreadPriorityPool::append(ThreadTask task, int nPriority)
{
	if (!m_bRunning.load())
	{
		return false; // Thread pool is not running
	}

	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_taskList.emplace(nPriority, std::move(task));
	}

	m_condition_empty.notify_one();
	return true;
}

ThreadTask ThreadPriorityPool::get_one_task()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_condition_empty.wait(lock, [this]() { return !m_taskList.empty() || !m_bRunning.load(); });

	if (!m_bRunning.load())
	{
		return nullptr; // Thread pool is stopping, return nullptr
	}

	auto task = std::move(m_taskList.top().second);
	m_taskList.pop();

	return task;
}

bool ThreadPriorityPool::start()
{
	if (m_bRunning.exchange(true))
	{
		return false; // Thread pool is already running
	}

	for (int i = 0; i < m_nThreadNumber; i++)
	{
		m_vecThread[i] = std::make_shared<std::thread>(&ThreadPriorityPool::threadWork, this);
	}

	return true;
}

bool ThreadPriorityPool::stop()
{
	if (!m_bRunning.exchange(false))
	{
		return false; // Thread pool is not running
	}

	m_condition_empty.notify_all();

	for (auto& thread : m_vecThread)
	{
		if (thread->joinable())
		{
			thread->join();
		}
	}

	return true;
}

bool ThreadPriorityPool::IsRunning()
{
	return m_bRunning.load();
}

int ThreadPriorityPool::getThreadNum() const
{
	return m_nThreadNumber;
}

int ThreadPriorityPool::getCompletedTaskCount() const
{
	return m_nCompletedTasks.load();
}

void ThreadPriorityPool::threadWork()
{
	while (m_bRunning.load())
	{
		auto task = get_one_task();
		if (task)
		{
			task();
			++m_nCompletedTasks;
		}
	}
}
//
//
//void myTask(int arg1, const std::string& arg2)
//{
//	
//	std::cout << "Task executed with arg1 = " << arg1 << ", arg2 = " << arg2 << std::endl;
//}
//
//int main()
//{
//	// ��ȡ�̳߳�ʵ��
//	netlib::ThreadPool* threadPool = netlib::ThreadPool::GetInstance();
//
//	// ���̳߳��������
//	int arg1 = 42;
//	std::string arg2 = "Hello, world!";
//	threadPool->append(myTask, arg1, arg2);
//
//	// ��������...
//
//	return 0;
//}