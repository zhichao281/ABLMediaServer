/*
功能：
  用epoll监听外部连接进行的客户，然后把客户端投递到读取线程池进行读取网络数据

日期    2025-07-12
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "ListenPool.h"
#include "identifier_generator.h"
#include "client_manager.h"
#include "ClientSendPool.h"
#ifndef OS_System_Windows
#include <fcntl.h>
#endif

extern CClientReadPool*          clientReadPool;
extern CClientSendPool*          clientSendPool;

CListenPool::CListenPool(int nThreadCount)
{
	nProcThreadOrder = 0;
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > MaxListenThreadCount)
		nTrueNetThreadPoolCount = MaxListenThreadCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;
 
	nGetCurrentThreadOrder = 0;
	unsigned long dwThread;
#ifdef USE_BOOST
	bRunFlag.store(true);
#else
	bRunFlag.store(true);
#endif
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
	}
}

int   CListenPool::GetThreadOrder()
{
	std::lock_guard<std::mutex> lock(threadLock);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder  ++;
	return nGet;
}

CListenPool::~CListenPool()
{
 
}

void CListenPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
 	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	uint64_t nClientID = 0 ;
	int      ret_num;
	int      i;
	int      ret_recv;
	struct   sockaddr_in client_addr;
	socklen_t addrLength = sizeof(client_addr);
	SOCKET   cfd;
	int      ret;
	int      size = 1024 * 1024 * 1;
	u_long   iMode = 1;
	struct timeval timeOut = { 3,0 };
	socklen_t      timeOutLength = sizeof(timeOut);

	//创建epoll句柄epfd
	epfd[nCurrentThreadID] = epoll_create(1);
	//printf(" CListenPool::ProcessFunc() nCurrentThreadID = %d \r\n", nCurrentThreadID);
	
	while (bRunFlag.load())
	{
		ret_num = epoll_wait(epfd[nCurrentThreadID], events[nCurrentThreadID], MaxEventCount,1000);
		if (ret_num > 0)
		{
			for (i = 0; i < ret_num; i++) 
			{//根据回调回来的 u64 进行查找 
				ListenStructPtr ptr = GetPtrBySvrHandle(events[nCurrentThreadID][i].data.u64);
				if (ptr == NULL)
				{
 					continue;
				}

				//把socket通过accept接入
 				cfd = accept(ptr->svrSocket, (sockaddr*)&client_addr, &addrLength);
				if (cfd == -1)
				{
 					continue;
				}

				//设置发送、接收缓冲区 
				ret = setsockopt(cfd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
				ret = setsockopt(cfd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
#ifdef OS_System_Windows
				ret = ioctlsocket(cfd, FIONBIO, &iMode);
#else
	            ret = ::fcntl(cfd, F_SETFL, O_NONBLOCK) ;
#endif	
				//设置发送、接收超时 
				ret = setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeOut, timeOutLength);
				ret = setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, timeOutLength);

				//设置不允许延时关闭
				struct linger sLinger;
				sLinger.l_onoff = 0;
				sLinger.l_linger = 1;
				int ret = setsockopt(cfd, SOL_SOCKET, SO_LINGER, (char *)&sLinger, sizeof(linger));

#ifdef USE_BOOST
				client_ptr cli = client_manager_singleton::get_mutable_instance().malloc_client(ptr->srvhandle, ptr->fnread, ptr->fnclose, (0 != ptr->autoread) ? true : false, ptr->bSSLFlag, clientType_Accept, ptr->fnaccept);
#else
				client_ptr cli = client_manager::get_instance().malloc_client(ptr->srvhandle, ptr->fnread, ptr->fnclose, (0 != ptr->autoread) ? true : false, ptr->bSSLFlag, clientType_Accept, ptr->fnaccept);
#endif
				if (cli)
				{
  					cli->m_Socket  = cfd;
					memcpy((char*)&cli->tAddr4, (char*)&client_addr, sizeof(client_addr));
					cli->SetConnect(true);
  
					//加入客户链表
#ifdef USE_BOOST
 					client_manager_singleton::get_mutable_instance().push_client(cli);
#else
 					client_manager::get_instance().push_client(cli);
#endif

					//加入发送线程池
					clientSendPool->InsertIntoTask(cli->get_id());
 
					//先通知上层连接成功 
					if (ptr->fnaccept)
						ptr->fnaccept(ptr->srvhandle, cli->get_id(), (void*)&client_addr);
					//加入读取线程池 
					clientReadPool->InsertIntoTask(cli->get_id());
   				}
				else
				{
#ifdef OS_System_Windows					
					closesocket(cfd);
#else 
	                close(cfd) ;
#endif	
				}
			}
		}
		else
			Sleep(5);
  	}
	
	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CListenPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CListenPool* pThread = (CListenPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

void CListenPool::destroyListenPool()
{
	bRunFlag.store(false);
	std::lock_guard<std::mutex> lock(threadLock);

	//关闭所有epoll , 并且等待退出所有线程 
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		if (epfd[i] != NULL)
		{
#ifdef OS_System_Windows
			epoll_close(epfd[i]);
#else
			close(epfd[i]);
#endif	
		}
		while (!bExitProcessThreadFlag[i].load())
			Sleep(100);
	}

	//关闭 accept 以防外部没有调用 UnListen() 函数 
	ListenStructPtrMap::iterator it;
	int nSize = 0 ;
	struct epoll_event  event;

	for (it = m_listenStructPtrMap.begin(); it != m_listenStructPtrMap.end();)
	{
  	    //执行关闭accept接入进来所有的连接SOCKET 
#ifdef USE_BOOST
		client_manager_singleton::get_mutable_instance().pop_server_clients((*it).second->srvhandle);
#else
		client_manager::get_instance().pop_server_clients((*it).second->srvhandle);
#endif
 
		//关闭accept的Socket套接字 
#ifdef OS_System_Windows
		closesocket((*it).second->svrSocket);
#else 
	    close((*it).second->svrSocket);
#endif
		m_listenStructPtrMap.erase(it++);
	}
}

int32_t CListenPool::Listen(
	int8_t* localip,
	uint16_t localport,
	NETHANDLE* srvhandle,
	accept_callback fnaccept,
	read_callback fnread,
	close_callback fnclose,
	uint8_t autoread,
	bool bSSLFlag
)
{
	*srvhandle = 0 ;

	std::shared_ptr<ListenStruct> listenStruct = std::make_shared<ListenStruct>();
	if (listenStruct == NULL)
		return e_libnet_err_make_shared;

	listenStruct->srvhandle = generate_identifier();
	listenStruct->autoread = autoread;
	listenStruct->bSSLFlag = bSSLFlag;
	listenStruct->fnaccept = fnaccept;
	listenStruct->fnread   = fnread;
	listenStruct->fnclose  = fnclose;
	strcpy((char*)listenStruct->localip,(char*)localip);
	listenStruct->localport = localport;
	listenStruct->nProcThreadOrder = nProcThreadOrder.load() % nTrueNetThreadPoolCount ;

	//绑定
	listenStruct->svrSocket = socket(AF_INET, SOCK_STREAM, 0);
	memset((char*)&listenStruct->addr, 0x00, sizeof(listenStruct->addr));
	listenStruct->addr.sin_family = AF_INET;
	listenStruct->addr.sin_addr.s_addr = inet_addr((char*)listenStruct->localip);
	listenStruct->addr.sin_port = htons(localport);

	int32_t   nReUse = 1;
 	setsockopt(listenStruct->svrSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#ifndef OS_System_Windows
	setsockopt(listenStruct->svrSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#endif 

	//设置不允许延时关闭
	struct linger sLinger;
	sLinger.l_onoff  = 0;
	sLinger.l_linger = 1;
	int nret = setsockopt(listenStruct->svrSocket, SOL_SOCKET, SO_LINGER, (char *)&sLinger, sizeof(linger));

	nret = ::bind(listenStruct->svrSocket, (sockaddr*)&listenStruct->addr, sizeof(listenStruct->addr));
	if (nret != 0)
	{
#ifdef OS_System_Windows
		closesocket(listenStruct->svrSocket);
#else 
	    close(listenStruct->svrSocket);
#endif		
		listenStruct.reset();
		return e_libnet_err_srvlistensockbind;
	}


	//监听 
	nret = listen(listenStruct->svrSocket, 256);
	if (nret == -1)
	{
#ifdef OS_System_Windows
		closesocket(listenStruct->svrSocket);
#else 
	    close(listenStruct->svrSocket);
#endif	
		listenStruct.reset();
		return e_libnet_err_srvlistenstart;
	}

	//加入epoll 
	struct epoll_event  event; 
	event.events = EPOLLIN;
	event.data.u64 = listenStruct->srvhandle;
	nret = epoll_ctl(epfd[listenStruct->nProcThreadOrder], EPOLL_CTL_ADD, listenStruct->svrSocket, &event);
	if (nret == -1)
	{
#ifdef OS_System_Windows
		closesocket(listenStruct->svrSocket);
#else 
	    close(listenStruct->svrSocket);
#endif			
		listenStruct.reset();
		return e_libnet_err_epoll_ctl;
	}

	std::lock_guard<std::mutex> lock(threadLock);

	auto ret =
		m_listenStructPtrMap.insert(std::make_pair(listenStruct->srvhandle, listenStruct));

	nProcThreadOrder.fetch_add(1);

	*srvhandle = listenStruct->srvhandle ;

	//printf("svrHandle = %d, Socket = %d \r\n", listenStruct->srvhandle, listenStruct->svrSocket);

 	return e_libnet_err_noerror;
}

int32_t CListenPool::Unlisten(NETHANDLE srvhandle)
{
	if (bRunFlag.load() == false)
		return e_libnet_err_clisocknotopen;

	std::lock_guard<std::mutex> lock(threadLock);

	ListenStructPtrMap::iterator it;
	int                          nSize;
	it = m_listenStructPtrMap.find(srvhandle);
	if (it != m_listenStructPtrMap.end())
	{
		//执行关闭accept接入进来所有的连接SOCKET 
#ifdef USE_BOOST
		client_manager_singleton::get_mutable_instance().pop_server_clients((*it).second->srvhandle);
#else
		client_manager::get_instance().pop_server_clients((*it).second->srvhandle);
#endif

		//把 accept 的socket 从 epoll 里面移除 
		struct epoll_event  event;
		event.events        = EPOLLIN;
		event.data.u64      = (*it).second->srvhandle;
		int nRet = epoll_ctl(epfd[(*it).second->nProcThreadOrder], EPOLL_CTL_DEL, (*it).second->svrSocket, &event);

		//关闭accept的Socket套接字 
#ifdef OS_System_Windows
		closesocket((*it).second->svrSocket);
#else 
		close((*it).second->svrSocket);
#endif	
		m_listenStructPtrMap.erase(it);

		return e_libnet_err_noerror;
	}
	else
		return e_libnet_err_invalidhandle;
}

ListenStructPtr CListenPool::GetPtrBySvrHandle(uint64_t nSvrHandle)
{
	if (bRunFlag.load() == false)
		return NULL;

	std::lock_guard<std::mutex> lock(threadLock);

	ListenStructPtrMap::iterator it;
	it = m_listenStructPtrMap.find(nSvrHandle);
	if (it != m_listenStructPtrMap.end())
	{
 		return (*it).second ; 
  	}
	else
		return NULL ;
}