#ifndef _NetBaseThreadPool_H
#define _NetBaseThreadPool_H

#ifdef USE_BOOST
#include <boost/lockfree/queue.hpp>
#include <condition_variable> 

typedef boost::unordered_map<NETHANDLE, NETHANDLE>   ClientProcessThreadMap;//固定客户端的线程序号 
#else
#include <queue>
#include <condition_variable> 

typedef map<NETHANDLE, NETHANDLE>   ClientProcessThreadMap;//固定客户端的线程序号 
#endif

#define    MaxNetHandleQueueCount     256 
class CNetBaseThreadPool
{
public:
	CNetBaseThreadPool(int nThreadCount);
   ~CNetBaseThreadPool();

   //插入任务ID 
   bool       InsertIntoTask(uint64_t nClientID);
   uint64_t   PopFromTask(int nThreadOrder);
   bool       DeleteFromTask(uint64_t nClientID);

private:
	volatile int nGetCurrentThreadOrder;
	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	volatile   uint64_t     nThreadProcessCount;
	std::mutex              threadLock;
	ClientProcessThreadMap  clientThreadMap;
    uint64_t                nTrueNetThreadPoolCount; 
    list<uint64_t>          m_NetHandleQueue[MaxNetHandleQueueCount];
	uint64_t                nGetCurClientID[MaxNetHandleQueueCount];
	volatile bool           bExitProcessThreadFlag[MaxNetHandleQueueCount];
    volatile bool           bCreateThreadFlag;
#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[MaxNetHandleQueueCount];
#endif
	volatile  bool        bRunFlag;

	std::condition_variable  cv[MaxNetHandleQueueCount];
	std::mutex               mtx[MaxNetHandleQueueCount];
};

#endif