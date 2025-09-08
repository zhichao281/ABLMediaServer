#ifndef _ConnectCheckPool_H
#define _ConnectCheckPool_H

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

#include   "client.h"

#define    CheckPool_MaxNetHandleQueueCount     1
#define    MaxConnectCheckEventCount            1024

struct ConectResultNote
{
	uint64_t       nClient;      //客户端ID
	unsigned short nPort;        //本地端口号
	bool           bConnectFlag; //是否连接成功 
};

class CConnectCheckPool
{
public:
	CConnectCheckPool(int nThreadCount);
   ~CConnectCheckPool();

   void                  destoryPool();
   EPOLLHANDLE           epfd;
   struct  epoll_event   events[MaxConnectCheckEventCount];

#ifdef USE_BOOST
   boost::unordered_map<NETHANDLE, NETHANDLE > clientMap;
   boost::atomic_bool    bRunFlag;
#else
   std::unordered_map<NETHANDLE, NETHANDLE > clientMap;
   std::atomic_bool      bRunFlag;
#endif

   bool                  InsertIntoTask(uint64_t nClientID);
   bool                  DeleteFromTask(uint64_t nClientID);
   void                  CheckTimeoutClient();
private:
	int         nGetCurrentThreadOrder;
	int         GetThreadOrder();
	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	volatile   uint64_t     nThreadProcessCount;
	std::mutex              threadLock;
    uint64_t                nTrueNetThreadPoolCount; 
	uint64_t                nGetCurClientID[CheckPool_MaxNetHandleQueueCount];
#ifdef USE_BOOST
	boost::atomic_bool      bExitProcessThreadFlag[CheckPool_MaxNetHandleQueueCount];
#else
	std::atomic_bool        bExitProcessThreadFlag[CheckPool_MaxNetHandleQueueCount];
#endif
    volatile bool           bCreateThreadFlag;
#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[CheckPool_MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[CheckPool_MaxNetHandleQueueCount];
#endif

};

#endif