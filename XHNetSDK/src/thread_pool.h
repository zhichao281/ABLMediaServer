#pragma once


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <future>

#include <stdexcept>


#define MAX_THREAD  256


namespace netlib
{
	using ThreadTask = std::function<void(void)>;
	
	class ThreadPool
	{
	public:				
		ThreadPool(int threadNumber);


		//������������������
		bool append(ThreadTask task, bool bPriority = false);	

		//template<typename Func, typename... Args>
		//void appendArg(Func&& func, Args&&... args)
		//{
		//	if (!m_bRunning.load())
		//	{
		//		return; // �̳߳�δ���У���ִ���κβ���
		//	}
		//	{
		//		std::lock_guard<std::mutex> guard(m_mutex);
		//		// ͨ��lambda����������Ͳ�����װ��һ�𣬲��洢�����������
		//		m_taskList.emplace([func = std::forward<Func>(func), args = std::make_tuple(std::forward<Args>(args)...)]() {
		//			std::apply(func, args);
		//		});
		//	}

		//	m_condition_empty.notify_one();
		//}


		ThreadTask get_one_task();
		//�����̳߳�
		bool start(void);

		//ֹͣ�̳߳�
		bool stop(void);

		// �̳߳��Ƿ�������
		bool IsRunning();

		int getThreadNum() const;

		int getCompletedTaskCount() const;
public:

		// ��ȡ�̳߳�ʵ��
		static ThreadPool& getInstance();
	private:

		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;

		ThreadPool& operator=(const ThreadPool&) = delete;	

private:
		//�߳���ִ�еĹ�������
		void threadWork(void);

private:
		std::mutex m_mutex;                                        //������	
	
		std::atomic< bool> m_bRunning;                              //�̳߳��Ƿ�������
		int m_nThreadNumber;                                       //�߳���

		std::condition_variable_any m_condition_empty;             //���������Ϊ��ʱ����������
		std::queue<ThreadTask> m_taskList;                          //�������
		
		std::vector<std::shared_ptr<std::thread>> m_vecThread;     //���������̶߳���ָ��
		//�����߳�����
		std::atomic<int>  m_idlThrNum;
		std::atomic<int> m_nCompletedTasks{ 0 };

	};


	struct ComparePriority
	{
		bool operator()(const std::pair<int, ThreadTask>& lhs, const std::pair<int, ThreadTask>& rhs) const
		{
			return lhs.first < rhs.first;
		}
	};



	class ThreadPriorityPool
	{
	public:
		ThreadPriorityPool(int threadNumber);
	;

		// ��������������
		//threadPool.append(task1, 1); // �е����ȼ�
		//threadPool.append(task2, 0); // �����ȼ�
		//threadPool.append(task3, 2); // �����ȼ�
		bool append(ThreadTask task, int nPriority=0);

		template<typename Func, typename... Args>
		void appendArg(Func&& func, Args&&... args);

		ThreadTask get_one_task();

		bool start();
		bool stop();
		bool IsRunning();

		int getThreadNum() const;
		int getCompletedTaskCount() const;

	public:
		static ThreadPriorityPool& getInstance() {
			static ThreadPriorityPool instance;
			return instance;
		}
	private:
		ThreadPriorityPool() = default;
		~ThreadPriorityPool();
		ThreadPriorityPool(const ThreadPriorityPool&) = delete;
		ThreadPriorityPool& operator=(const ThreadPriorityPool&) = delete;	

	private:
		void threadWork();

	private:
		std::mutex m_mutex;
		std::atomic<bool> m_bRunning;
		int m_nThreadNumber;
		std::condition_variable_any m_condition_empty;
		std::vector<std::shared_ptr<std::thread>> m_vecThread;
		std::priority_queue<std::pair<int, ThreadTask>, std::vector<std::pair<int, ThreadTask>>, ComparePriority> m_taskList;
		std::atomic<int> m_nCompletedTasks{ 0 };
		static ThreadPriorityPool* s_pThreadPool;
	};


}





