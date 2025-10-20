#ifndef _UdpPool_H
#define _UdpPool_H


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

#define     MaxUdpThreadCount     256 
#define     MaxUdpEventCount      1024 

#ifndef EPOLLHANDLE
#define EPOLLHANDLE  int 
#endif 

struct UdpStruct
{
	int8_t          localip[128] = { 0 };
	uint16_t        localport;
	NETHANDLE       srvhandle;
 	read_callback   fnread = NULL ;
    uint8_t         autoread;
 	SOCKET          udpSocket;
	sockaddr_in     addr;

	int             nProcThreadOrder;//处理线程序号
};


#ifdef USE_BOOST
typedef boost::shared_ptr<UdpStruct> UdpStructPtr;
typedef boost::unordered_map<NETHANDLE, UdpStructPtr> UdpStructPtrMap;  //存储listen结构的智能指针

#else
typedef std::shared_ptr<UdpStruct> UdpStructPtr;
typedef std::unordered_map<NETHANDLE, UdpStructPtr> UdpStructPtrMap;  //存储listen结构的智能指针

#endif


class CUdpPool
{
public:
	CUdpPool(int nThreadCount);
   ~CUdpPool();
  	void destoryUdpPool();


   int32_t BuildUdp(
		int8_t* localip,
		uint16_t localport,
	   void* bindaddr,
		NETHANDLE* srvhandle,
 		read_callback fnread,
 		uint8_t autoread
 	);
   int32_t Sendto(NETHANDLE udphandle,
	   uint8_t* data,
	   uint32_t datasize,
	   void* remoteaddress);
   int32_t      DestoryUdp(NETHANDLE srvhandle);
   UdpStructPtr GetUdpPtrHandle(uint64_t nSvrHandle);
  
   EPOLLHANDLE                                  epfd[MaxUdpThreadCount];
   struct  epoll_event                          events[MaxUdpThreadCount][MaxUdpEventCount];
   UdpStructPtrMap                              m_UdpStructPtrMap;



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
	boost::atomic<uint64_t> nProcThreadOrder;
	boost::atomic_bool     bRunFlag;
	boost::atomic_bool      bExitProcessThreadFlag[MaxNetHandleQueueCount];
	boost::atomic_bool      bCreateThreadFlag;
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