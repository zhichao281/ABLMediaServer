#ifndef _ClientSendPool_H
#define _ClientSendPool_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/atomic/atomic.hpp>

#include   "client.h"
#define     MaxNetHandleQueueCount     256 
#define     MaxEventCount              1024 

#ifndef EPOLLHANDLE
#define EPOLLHANDLE  int 
#endif 

class CClientSendPool
{
public:
	CClientSendPool(int nThreadCount);
   ~CClientSendPool();
   
   void                                         destroySendPool();
   boost::unordered_map<NETHANDLE, client_ptr > clientMap[MaxNetHandleQueueCount];  //把客户存储起来 
   std::condition_variable                      cv[MaxNetHandleQueueCount];
   std::mutex                                   mtx[MaxNetHandleQueueCount];
   bool                                         notify_one(int nThreadOrder);
   std::mutex                                   threadLock[MaxNetHandleQueueCount];

   //插入任务ID 
   uint32_t           GetSendOrder();//获取发送线程序号
   std::mutex         getOrderMutex;

   bool               InsertIntoTask(uint64_t nClientID);
   bool               DeleteFromTask(int nThreadOrder,uint64_t nClientID);
   boost::atomic_bool bRunFlag;

private:
	int                   GetThreadOrder();
	int                   nGetCurrentThreadOrder;

	void         ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	boost::atomic<uint64_t> nThreadProcessCount;
    uint64_t                nTrueNetThreadPoolCount; 
	uint64_t                nGetCurClientID[MaxNetHandleQueueCount];
	boost::atomic_bool      bExitProcessThreadFlag[MaxNetHandleQueueCount];
	boost::atomic_bool      bCreateThreadFlag;
#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[MaxNetHandleQueueCount];
#endif

};

#endif