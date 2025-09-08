#ifndef _UdpPool_H
#define _UdpPool_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/atomic.hpp>

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

	int             nProcThreadOrder;//�����߳����
};

typedef boost::shared_ptr<UdpStruct> UdpStructPtr;
typedef boost::unordered_map<NETHANDLE, UdpStructPtr> UdpStructPtrMap;  //�洢listen�ṹ������ָ��

class CUdpPool
{
public:
	CUdpPool(int nThreadCount);
   ~CUdpPool();
  	void destoryUdpPool();

	boost::atomic<uint64_t> nProcThreadOrder;
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