#ifndef _ListenPool_H
#define _ListenPool_H


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


#ifdef USE_BOOST
typedef boost::shared_ptr<ListenStruct> ListenStructPtr;
typedef boost::unordered_map<NETHANDLE, ListenStructPtr> ListenStructPtrMap;  //存储listen结构的智能指针

#else
typedef std::shared_ptr<ListenStruct> ListenStructPtr;
typedef std::unordered_map<NETHANDLE, ListenStructPtr> ListenStructPtrMap;  //存储listen结构的智能指针

#endif

class CListenPool
{
public:
	CListenPool(int nThreadCount);
   ~CListenPool();
  	void destroyListenPool();


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


private:
	int                   GetThreadOrder();
	int                   nGetCurrentThreadOrder;

	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);

	volatile   uint64_t     nThreadProcessCount;
	std::mutex              threadLock;
    uint64_t                nTrueNetThreadPoolCount; 
	uint64_t                nGetCurClientID[MaxNetHandleQueueCount];

#ifdef USE_BOOST
	boost::atomic<bool>      bExitProcessThreadFlag[MaxNetHandleQueueCount];
	boost::atomic<bool>      bCreateThreadFlag;
	boost::atomic<uint64_t>  nProcThreadOrder;
	boost::atomic<bool>      bRunFlag;
#else
	std::atomic<bool>        bExitProcessThreadFlag[MaxNetHandleQueueCount];
	std::atomic<bool>        bCreateThreadFlag;
	std::atomic<uint64_t>    nProcThreadOrder;
	std::atomic<bool>        bRunFlag;
#endif

#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[MaxNetHandleQueueCount];
#endif

};

#endif