/*
功能：
  用epoll监听client发起的连接是否成功，如果成功通知连接成功并投递到线程池，如果超时则通知连接失败

日期    2025-07-15
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/
#include "stdafx.h"
#include "ConnectCheckPool.h"
#include "client_manager.h"
#include "ClientSendPool.h"

extern CClientReadPool*          clientReadPool ;
extern CClientSendPool*          clientSendPool ;

CConnectCheckPool::CConnectCheckPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount;
	if (nThreadCount > CheckPool_MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = CheckPool_MaxNetHandleQueueCount;

	nTrueNetThreadPoolCount = CheckPool_MaxNetHandleQueueCount ;

	nGetCurrentThreadOrder = 0;
	unsigned long dwThread;
	bRunFlag.store(true);
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		bCreateThreadFlag = false;
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
	}
}

CConnectCheckPool::~CConnectCheckPool()
{

}

void   CConnectCheckPool::destoryPool()
{
	std::lock_guard<std::mutex> lock(threadLock);
	bRunFlag.store(false);
#ifdef OS_System_Windows
	epoll_close(epfd);
#else 
	close(epfd);
#endif
	int i;

	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		while (!bExitProcessThreadFlag[i].load())
			Sleep(50);
#ifdef  OS_System_Windows
		CloseHandle(hProcessHandle[i]);
#endif 
	}
}

int   CConnectCheckPool::GetThreadOrder()
{
	std::lock_guard<std::mutex> lock(threadLock);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder++;
	return nGet;
}

void CConnectCheckPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
 	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	uint64_t nClientID = 0 ;
	ConectResultNote resultNote;
	unsigned char*   pData;
	int              nLength;
	int              ret_num;
	int              ret;
	int              size = 1024 * 1024 * 2;
	int              opt = 1 ;
	struct sockaddr_in m_sockaddr_in;
	socklen_t        nSize =sizeof(sockaddr_in);
	bool             bDeleteFlag = false;

	bCreateThreadFlag = true; //创建线程完毕
	epfd  = epoll_create(1);
	while (bRunFlag.load() == true )
	{
		ret_num = epoll_wait(epfd, events, MaxConnectCheckEventCount, 1000);
		if (ret_num > 0)
		{
			for (int i = 0; i < ret_num; i++)
			{
			   bDeleteFlag = false;
			   client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(events[i].data.u64);
			   if (cli)
			   {
				   if (events[i].events == EPOLLOUT)
				   {//连接成功
 					   ret = setsockopt(cli->m_Socket, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
					   ret = setsockopt(cli->m_Socket, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
					   ret = setsockopt(cli->m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));

					   bDeleteFlag = true;
					   cli->SetConnect(true);
					   clientReadPool->InsertIntoTask(cli->get_id());
					   clientSendPool->InsertIntoTask(cli->get_id());
					   if (cli->m_fnconnect)
					   {
						   ::getsockname(cli->m_Socket, (sockaddr*)&m_sockaddr_in, &nSize);
						   cli->m_fnconnect(cli->get_id(), 1, ntohs(m_sockaddr_in.sin_port));
					   }
				   }
				   else
				   {
					   if (cli->m_nClientType == clientType_Connect && (GetTickCount64() - cli->nCreateTime >= 1000 * 8))
					   {//在linux、arm平台汇报别的错误消息，但是尚未达到连接超时的时间，不能删除epoll检测
						   bDeleteFlag = true;
						   client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());
					      if(cli->m_fnconnect)
					        cli->m_fnconnect(cli->get_id(), 0, ntohs(cli->tAddr4.sin_port));
 					   }
				   }

				   if (bDeleteFlag == true)
				   {//从epoll移除，不再需要对该socket进行监测是否可写
 					   struct epoll_event  event;
					   event.events = EPOLLOUT;
					   event.data.u64 = cli->get_id();
					   int nRet = epoll_ctl(epfd, EPOLL_CTL_DEL, cli->m_Socket, &event);

					   //删除ID
					   DeleteFromTask(cli->get_id());
				   }
			   }
			   else
			   {//删除

			   }
			}
		}

		//判断连接超时的client 
		CheckTimeoutClient();
   	}

	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CConnectCheckPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CConnectCheckPool* pThread = (CConnectCheckPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

//把需要连接检测的ID记录下来，并且进行epoll监听
bool CConnectCheckPool::InsertIntoTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;

	std::lock_guard<std::mutex> lock(threadLock);
	client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(nClientID);
	if (cli)
	{
		//记录检测连接是否成功、连接超时的ID 
		clientMap.insert(std::make_pair(nClientID, nClientID));

		struct epoll_event  event;
		event.events = EPOLLOUT;
		event.data.u64 = cli->get_id();
		int nRet = epoll_ctl(epfd, EPOLL_CTL_ADD, cli->m_Socket, &event);
 
		nRet = ::connect(cli->m_Socket, (sockaddr*)&cli->tAddr4, sizeof(cli->tAddr4));

		return true;
	}

	return false ;
}

//把检测完毕的ID移除掉 
bool CConnectCheckPool::DeleteFromTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false ;

	std::lock_guard<std::mutex> lock(threadLock);

	bool       bRet = false;
#ifdef USE_BOOST
	boost::unordered_map<NETHANDLE, NETHANDLE >::iterator it;
	it = clientMap.find(nClientID);
	if (it != clientMap.end())
	{
		clientMap.erase(it);

		bRet = true;
	}
	else
		bRet = false;
#else
	std::unordered_map<NETHANDLE, NETHANDLE >::iterator it;
	it = clientMap.find(nClientID);
	if (it != clientMap.end())
	{
		clientMap.erase(it);

		bRet = true;
	}
	else
		bRet = false;
#endif


	return bRet;
}

//判断连接超时的client，执行删除，通知连接失败
void  CConnectCheckPool::CheckTimeoutClient()
{
	if (bRunFlag.load() == false)
		return;
 	std::lock_guard<std::mutex> lock(threadLock);
#ifdef USE_BOOST
	boost::unordered_map<NETHANDLE, NETHANDLE >::iterator it; 
	for (it = clientMap.begin(); it != clientMap.end();)
	{
		client_ptr cli = client_manager_singleton::get_mutable_instance().get_client((*it).second);
		if (cli != NULL)
		{
			if (cli->GetConnect() == false && cli->m_nClientType == clientType_Connect && (GetTickCount64() - cli->nCreateTime >= 1000 * 8) )
			{
				//从epoll里面移除socket,不再需要监测
				struct epoll_event  event;
				event.events = EPOLLOUT;
				event.data.u64 = cli->get_id();
				int nRet = epoll_ctl(epfd, EPOLL_CTL_DEL, cli->m_Socket, &event);
 
 				//先删除客户端
				client_manager_singleton::get_mutable_instance().pop_client((*it).second);

				//通知连接失败
				if(cli->m_fnconnect)
				   cli->m_fnconnect(cli->get_id(), 0, ntohs(cli->tAddr4.sin_port));
 
				//再删除ID
				clientMap.erase(it ++);
			}
			else
				it ++;
		}
		else
			clientMap.erase(it++);
	}
#else
	std::unordered_map<NETHANDLE, NETHANDLE >::iterator it; 
	for (it = clientMap.begin(); it != clientMap.end();)
	{
		client_ptr cli = client_manager_singleton::get_mutable_instance().get_client((*it).second);
		if (cli != NULL)
		{
			if (cli->GetConnect() == false && cli->m_nClientType == clientType_Connect && (GetTickCount64() - cli->nCreateTime >= 1000 * 8) )
			{
				//从epoll里面移除socket,不再需要监测
				struct epoll_event  event;
				event.events = EPOLLOUT;
				event.data.u64 = cli->get_id();
				int nRet = epoll_ctl(epfd, EPOLL_CTL_DEL, cli->m_Socket, &event);
 
 				//先删除客户端
				client_manager_singleton::get_mutable_instance().pop_client((*it).second);

				//通知连接失败
				if(cli->m_fnconnect)
				   cli->m_fnconnect(cli->get_id(), 0, ntohs(cli->tAddr4.sin_port));
 
				//再删除ID
				clientMap.erase(it ++);
			}
			else
				it ++;
		}
		else
			clientMap.erase(it++);
	}
#endif
	
}
