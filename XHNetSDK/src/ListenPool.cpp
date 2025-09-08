/*
���ܣ�
  ��epoll�����ⲿ���ӽ��еĿͻ���Ȼ��ѿͻ���Ͷ�ݵ���ȡ�̳߳ؽ��ж�ȡ��������

����    2025-07-12
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "ListenPool.h"
#include "identifier_generator.h"
#include "client_manager.h"
#include "ClientSendPool.h"

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
	bRunFlag = true;
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

	//����epoll���epfd
	epfd[nCurrentThreadID] = epoll_create(1);
	//printf(" CListenPool::ProcessFunc() nCurrentThreadID = %d \r\n", nCurrentThreadID);
	
	while (bRunFlag.load())
	{
		ret_num = epoll_wait(epfd[nCurrentThreadID], events[nCurrentThreadID], MaxEventCount,1000);
		if (ret_num > 0)
		{
			for (i = 0; i < ret_num; i++) 
			{//���ݻص������� u64 ���в��� 
				ListenStructPtr ptr = GetPtrBySvrHandle(events[nCurrentThreadID][i].data.u64);
				if (ptr == NULL)
				{
 					continue;
				}

				//��socketͨ��accept����
 				cfd = accept(ptr->svrSocket, (sockaddr*)&client_addr, &addrLength);
				if (cfd == -1)
				{
 					continue;
				}

				//���÷��͡����ջ����� 
				ret = setsockopt(cfd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
				ret = setsockopt(cfd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
#ifdef OS_System_Windows
				ret = ioctlsocket(cfd, FIONBIO, &iMode);
#else
	            ret = ::fcntl(cfd, F_SETFL, O_NONBLOCK) ;
#endif	
				//���÷��͡����ճ�ʱ 
				ret = setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeOut, timeOutLength);
				ret = setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeOut, timeOutLength);

				//���ò�������ʱ�ر�
				struct linger sLinger;
				sLinger.l_onoff = 0;
				sLinger.l_linger = 1;
				int ret = setsockopt(cfd, SOL_SOCKET, SO_LINGER, (char *)&sLinger, sizeof(linger));

				client_ptr cli = client_manager_singleton::get_mutable_instance().malloc_client(ptr->srvhandle, ptr->fnread, ptr->fnclose, (0 != ptr->autoread) ? true : false, ptr->bSSLFlag, clientType_Accept, ptr->fnaccept);
				if (cli)
				{
  					cli->m_Socket  = cfd;
					memcpy((char*)&cli->tAddr4, (char*)&client_addr, sizeof(client_addr));
					cli->SetConnect(true);
  
					//����ͻ�����
 					client_manager_singleton::get_mutable_instance().push_client(cli);

					//���뷢���̳߳�
					clientSendPool->InsertIntoTask(cli->get_id());
 
					//��֪ͨ�ϲ����ӳɹ� 
					if (ptr->fnaccept)
						ptr->fnaccept(ptr->srvhandle, cli->get_id(), (void*)&client_addr);
					//�����ȡ�̳߳� 
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
	pthread_detach(pthread_self()); //�����̺߳����̷߳��룬�����߳��˳�ʱ���Զ��ͷ����߳��ڴ�
#endif

	CListenPool* pThread = (CListenPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //�˳��߳�
#endif
	return  0;
}

void CListenPool::destroyListenPool()
{
	bRunFlag.store(false);
	std::lock_guard<std::mutex> lock(threadLock);

	//�ر�����epoll , ���ҵȴ��˳������߳� 
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

	//�ر� accept �Է��ⲿû�е��� UnListen() ���� 
	ListenStructPtrMap::iterator it;
	int nSize = 0 ;
	struct epoll_event  event;

	for (it = m_listenStructPtrMap.begin(); it != m_listenStructPtrMap.end();)
	{
  	    //ִ�йر�accept����������е�����SOCKET 
		client_manager_singleton::get_mutable_instance().pop_server_clients((*it).second->srvhandle);
 
		//�ر�accept��Socket�׽��� 
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

	boost::shared_ptr<ListenStruct> listenStruct = boost::make_shared<ListenStruct>();
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

	//��
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

	//���ò�������ʱ�ر�
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


	//���� 
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

	//����epoll 
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

	std::pair<boost::unordered_map<NETHANDLE, ListenStructPtr>::iterator, bool> ret =
		m_listenStructPtrMap.insert(std::make_pair(listenStruct->srvhandle, listenStruct));

	nProcThreadOrder.add(1);

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
		//ִ�йر�accept����������е�����SOCKET 
		client_manager_singleton::get_mutable_instance().pop_server_clients((*it).second->srvhandle);

		//�� accept ��socket �� epoll �����Ƴ� 
		struct epoll_event  event;
		event.events        = EPOLLIN;
		event.data.u64      = (*it).second->srvhandle;
		int nRet = epoll_ctl(epfd[(*it).second->nProcThreadOrder], EPOLL_CTL_DEL, (*it).second->svrSocket, &event);

		//�ر�accept��Socket�׽��� 
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