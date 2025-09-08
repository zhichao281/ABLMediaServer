#ifndef _ConnectCheckPool_H
#define _ConnectCheckPool_H

#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>

#include   "client.h"

#define    CheckPool_MaxNetHandleQueueCount     1
#define    MaxConnectCheckEventCount            1024

struct ConectResultNote
{
	uint64_t       nClient;      //�ͻ���ID
	unsigned short nPort;        //���ض˿ں�
	bool           bConnectFlag; //�Ƿ����ӳɹ� 
};

class CConnectCheckPool
{
public:
	CConnectCheckPool(int nThreadCount);
   ~CConnectCheckPool();

   void                  destoryPool();
   EPOLLHANDLE           epfd;
   struct  epoll_event   events[MaxConnectCheckEventCount];

   boost::unordered_map<NETHANDLE, NETHANDLE > clientMap;  //�����жϿͻ����Ƿ����ӳɹ�

   //��������ID 
   boost::atomic_bool    bRunFlag;
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
	boost::atomic_bool      bExitProcessThreadFlag[CheckPool_MaxNetHandleQueueCount];
    volatile bool           bCreateThreadFlag;
#ifdef  OS_System_Windows
    HANDLE                hProcessHandle[CheckPool_MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[CheckPool_MaxNetHandleQueueCount];
#endif

};

#endif