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


namespace ABL
{

	using ThreadTask = std::function<void(void)>;
	
	class ThreadPool
	{
	public:				
		ThreadPool(int threadNumber);


		//往任务队列里添加任务
		bool append(ThreadTask task, bool bPriority = false);	

		//template<typename Func, typename... Args>
		//void appendArg(Func&& func, Args&&... args)
		//{
		//	if (!m_bRunning.load())
		//	{
		//		return; // 线程池未运行，不执行任何操作
		//	}
		//	{
		//		std::lock_guard<std::mutex> guard(m_mutex);
		//		// 通过lambda函数将任务和参数封装在一起，并存储在任务队列中
		//		m_taskList.emplace([func = std::forward<Func>(func), args = std::make_tuple(std::forward<Args>(args)...)]() {
		//			std::apply(func, args);
		//		});
		//	}

		//	m_condition_empty.notify_one();
		//}


		ThreadTask get_one_task();
		//启动线程池
		bool start(void);

		//停止线程池
		bool stop(void);

		// 线程池是否在运行
		bool IsRunning();

		int getThreadNum() const;

		int getCompletedTaskCount() const;
public:

		// 获取线程池实例
		static ThreadPool& getInstance();
	private:

		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;

		ThreadPool& operator=(const ThreadPool&) = delete;	

private:
		//线程所执行的工作函数
		void threadWork(void);

private:
		std::mutex m_mutex;                                        //互斥锁	
	
		std::atomic< bool> m_bRunning;                              //线程池是否在运行
		int m_nThreadNumber;                                       //线程数

		std::condition_variable_any m_condition_empty;             //当任务队列为空时的条件变量
		std::queue<ThreadTask> m_taskList;                          //任务队列
		
		std::vector<std::shared_ptr<std::thread>> m_vecThread;     //用来保存线程对象指针
		//空闲线程数量
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

		// 添加任务到任务队列
		//threadPool.append(task1, 1); // 中等优先级
		//threadPool.append(task2, 0); // 低优先级
		//threadPool.append(task3, 2); // 高优先级
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





