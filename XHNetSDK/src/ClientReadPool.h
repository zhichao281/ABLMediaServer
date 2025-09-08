#ifndef _ClientReadPool_H
#define _ClientReadPool_H

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/atomic.hpp>
#else
#include <unordered_map>
#include <memory>
#include <atomic>
#endif

#define     MaxNetHandleQueueCount     256 
#define     MaxEventCount              1024 

#include   "client.h"
#ifndef EPOLLHANDLE
#define EPOLLHANDLE  int 
#endif 
struct ClientParamStruct
{
	read_callback    fnread;
	close_callback   fnclose;
	NETHANDLE        nClient;
	SOCKET           m_Socket;
	void*            pAddress;
};

class CClientReadPool
{
public:
	CClientReadPool(int nThreadCount);
   ~CClientReadPool();
   
   void                    destroyReadPool();
   EPOLLHANDLE                                  epfd[MaxNetHandleQueueCount];
   struct  epoll_event                          events[MaxNetHandleQueueCount][MaxEventCount];

   //≤Â»Î»ŒŒÒID 
   bool               InsertIntoTask(uint64_t nClientID);
   bool               DeleteFromTask(uint64_t nClientID);
#ifdef USE_BOOST
   boost::atomic_bool bRunFlag;
#else
   std::atomic_bool bRunFlag;
#endif

private:
	int                   GetThreadOrder();
	int                   nGetCurrentThreadOrder;

	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

#ifdef USE_BOOST
	boost::atomic_uint64_t  nThreadProcessCount;
	boost::atomic_bool      bExitProcessThreadFlag[MaxNetHandleQueueCount];
	boost::atomic_bool      bCreateThreadFlag;
#else
    std::atomic<uint64_t> nThreadProcessCount;
    std::atomic_bool bExitProcessThreadFlag[MaxNetHandleQueueCount];
    std::atomic_bool bCreateThreadFlag;
#endif
	int                   pad0[6]; // ensure 64byte cache alignment
	uint64_t                nTrueNetThreadPoolCount; 
	uint64_t                nGetCurClientID[MaxNetHandleQueueCount];
#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[MaxNetHandleQueueCount];
#endif

};

#endif