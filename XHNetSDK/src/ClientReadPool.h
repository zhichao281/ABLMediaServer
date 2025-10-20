#ifndef _ClientReadPool_H
#define _ClientReadPool_H

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#else
#include <unordered_map>
#include <memory>
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
	void* pAddress;
};

class CClientReadPool
{
public:
	CClientReadPool(int nThreadCount);
	~CClientReadPool();

	void                    destroyReadPool();
	EPOLLHANDLE                                  epfd[MaxNetHandleQueueCount];
	struct  epoll_event                          events[MaxNetHandleQueueCount][MaxEventCount];

	//插入任务ID 
	bool               InsertIntoTask(uint64_t nClientID);
	bool               DeleteFromTask(uint64_t nClientID);

	void handle_http_request(client_ptr cli,const std::string& request);
#ifdef USE_BOOST
	boost::atomic<bool> bRunFlag;
	boost::atomic_uint64_t nThreadProcessCount;
	boost::atomic<bool> bExitProcessThreadFlag[MaxNetHandleQueueCount];
	boost::atomic<bool> bCreateThreadFlag;
#else
	std::atomic<bool> bRunFlag;
	std::atomic<uint64_t> nThreadProcessCount;
	std::atomic<bool> bExitProcessThreadFlag[MaxNetHandleQueueCount];
	std::atomic<bool> bCreateThreadFlag;
#endif


private:
	int                   GetThreadOrder();
	int                   nGetCurrentThreadOrder;

	void ProcessFunc();
	static void* OnProcessThread(void* lpVoid);


	std::mutex              threadLock;
	uint64_t                nTrueNetThreadPoolCount;
	uint64_t                nGetCurClientID[MaxNetHandleQueueCount];

#ifdef  OS_System_Windows
	HANDLE                hProcessHandle[MaxNetHandleQueueCount];
#else
	pthread_t             hProcessHandle[MaxNetHandleQueueCount];
#endif

	std::string m_web_root = "html"; // 默认当前目录
	bool m_has_web_root = false;

};

#endif