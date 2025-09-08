#ifndef _ListenPool_H
#define _ListenPool_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/atomic.hpp>

#include <vector>

using namespace std;
#ifndef EPOLLHANDLE
#define EPOLLHANDLE  int 
#endif 

#define     MaxListenThreadCount     256 
#define     MaxListenEventCount      1024 

struct ListenStruct
{
	int8_t          localip[128] = { 0 };
	uint16_t        localport;
	NETHANDLE       srvhandle;
	accept_callback fnaccept = NULL;
	read_callback   fnread = NULL;
	close_callback  fnclose = NULL;
    uint8_t         autoread;
	bool            bSSLFlag  ;
	SOCKET          svrSocket;
	sockaddr_in     addr;

	int             nProcThreadOrder;//处理线程序号
};

typedef boost::shared_ptr<ListenStruct> ListenStructPtr;
typedef boost::unordered_map<NETHANDLE, ListenStructPtr> ListenStructPtrMap;  //存储listen结构的智能指针

class CListenPool
{
public:
	CListenPool(int nThreadCount);
   ~CListenPool();
  	void destroyListenPool();

	boost::atomic<uint64_t> nProcThreadOrder;
   int32_t Listen(
		int8_t* localip,
		uint16_t localport,
		NETHANDLE* srvhandle,
		accept_callback fnaccept,
		read_callback fnread,
		close_callback fnclose,
		uint8_t autoread,
		bool bSSLFlag 
	);
   int32_t Unlisten(NETHANDLE srvhandle);
   ListenStructPtr GetPtrBySvrHandle(uint64_t nSvrHandle);
  
   EPOLLHANDLE                                  epfd[MaxListenThreadCount];
   struct  epoll_event                          events[MaxListenThreadCount][MaxListenEventCount];
   ListenStructPtrMap                           m_listenStructPtrMap;

   boost::atomic_bool     bRunFlag;

private:
	int                   GetThreadOrder();
	int                   nGetCurrentThreadOrder;

	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	volatile   uint64_t     nThreadProcessCount;
	std::mutex              threadLock;
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