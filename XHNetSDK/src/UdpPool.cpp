/*
功能：
  用epoll监听udp对象进行数据读取，也支持udp发送数据

日期    2025-07-12
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "UdpPool.h"
#include "identifier_generator.h"
#include "client_manager.h"

extern CClientReadPool*          clientReadPool;

CUdpPool::CUdpPool(int nThreadCount)
{
	nProcThreadOrder = 0;
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount /  2;
	if (nThreadCount > MaxUdpThreadCount)
		nTrueNetThreadPoolCount = MaxUdpThreadCount;

	if (nThreadCount <= 0)
		nTrueNetThreadPoolCount = 64 ;
 
	nGetCurrentThreadOrder = 0;
	unsigned long dwThread;
	bRunFlag.store(true);
	for (int i = 0; i < nTrueNetThreadPoolCount; i++)
	{
#ifdef OS_System_Windows
		hProcessHandle[i] = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnProcessThread, (LPVOID)this, 0, &dwThread);
#else
		pthread_create(&hProcessHandle[i], NULL, OnProcessThread, (void*)this);
#endif
	}
}

int   CUdpPool::GetThreadOrder()
{
	if (bRunFlag.load() == false)
		return 0;
	std::lock_guard<std::mutex> lock(threadLock);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder  ++;
	return nGet;
}

CUdpPool::~CUdpPool()
{

}

void CUdpPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
 	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	uint64_t nClientID = 0 ;
	int      ret_num;
	int      i;
	int      ret_recv;
	struct   sockaddr_in client_addr;
	socklen_t      addrLength = sizeof(client_addr);
	SOCKET   cfd;
	int      ret;
	int      size = 1024 * 1024 * 2;
	u_long   iMode = 1;
	unsigned char*   udpData = new unsigned char[1024 * 64];

	//创建epoll句柄epfd
	epfd[nCurrentThreadID] = epoll_create(1);
	memset(udpData, 0x00, 1024 * 64);

	while (bRunFlag.load())
	{
		ret_num = epoll_wait(epfd[nCurrentThreadID], events[nCurrentThreadID], MaxUdpEventCount, 1000);
		if (ret_num > 0)
		{
			for (i = 0; i < ret_num; i++) 
			{//根据回调回来的 u64 进行查找 
				UdpStructPtr ptr = GetUdpPtrHandle(events[nCurrentThreadID][i].data.u64);
				if (ptr == NULL)
				{
 					continue;
				}

				ret_recv = recvfrom(ptr->udpSocket,(char*)udpData, 1024 * 64, 0, (sockaddr*)&client_addr, &addrLength);

				if (ptr->fnread && ret_recv > 0)
				{
					if (ret_recv < 1024 * 64)
						udpData[ret_recv] = 0x00;
					ptr->fnread(0, events[nCurrentThreadID][i].data.u64, udpData, static_cast<uint32_t>(ret_recv),(void*)&client_addr);
 				}

			}
		}
		else
			Sleep(5);
  	}
	delete udpData;
	udpData = NULL;
	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CUdpPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CUdpPool* pThread = (CUdpPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

void CUdpPool::destoryUdpPool()
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
		while (!bExitProcessThreadFlag[i])
			Sleep(100);
	}
	//关闭 accept 以防外部没有调用 UnListen() 函数 
	UdpStructPtrMap::iterator it;
	int nSize = 0 ;
	struct epoll_event  event;

	for (it = m_UdpStructPtrMap.begin(); it != m_UdpStructPtrMap.end();)
	{
    	//关闭accept的Socket套接字 
#ifdef OS_System_Windows
		closesocket((*it).second->udpSocket);
#else 
		close((*it).second->udpSocket);
#endif	
		m_UdpStructPtrMap.erase(it++);
	}

}

int32_t CUdpPool::BuildUdp(
	int8_t* localip,
	uint16_t localport,
	void* bindaddr,
	NETHANDLE* srvhandle,
	read_callback fnread,
	uint8_t autoread
)
{
	*srvhandle = 0 ;

	std::shared_ptr<UdpStruct> udpStruct = std::make_shared<UdpStruct>();
	if (udpStruct == NULL)
		return e_libnet_err_make_shared;

	udpStruct->srvhandle = generate_identifier();
	udpStruct->autoread = autoread;
 	udpStruct->fnread   = fnread;
	udpStruct->localport = localport;
	udpStruct->nProcThreadOrder = nProcThreadOrder.load() % nTrueNetThreadPoolCount ;
 
	//生成socket 
	udpStruct->udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpStruct->udpSocket <= 0 )
		return e_libnet_err_clicreate;

	//设置地址、端口允许重用
	int32_t   nReUse = 1;
	setsockopt(udpStruct->udpSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#ifndef OS_System_Windows
	setsockopt(udpStruct->udpSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&nReUse, static_cast<socklen_t>(sizeof(nReUse)));
#endif 

	//绑定
	memset((char*)&udpStruct->addr, 0x00, sizeof(udpStruct->addr));
	udpStruct->addr.sin_family = AF_INET;
	udpStruct->addr.sin_addr.s_addr = INADDR_ANY;
	udpStruct->addr.sin_port = htons(localport);
	int nret = ::bind(udpStruct->udpSocket, (sockaddr*)&udpStruct->addr, sizeof(udpStruct->addr));
	if (nret != 0)
	{
#ifdef OS_System_Windows
		closesocket(udpStruct->udpSocket);
#else 
	    close(udpStruct->udpSocket);
#endif
		udpStruct.reset();
		return e_libnet_err_clibind;
	}

	int size = 1024 * 1024 * 2;
	setsockopt(udpStruct->udpSocket, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	setsockopt(udpStruct->udpSocket, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));

	//加入epoll 
	struct epoll_event  event; 
	event.events = EPOLLIN;
	event.data.u64 = udpStruct->srvhandle;
	nret = epoll_ctl(epfd[udpStruct->nProcThreadOrder], EPOLL_CTL_ADD, udpStruct->udpSocket, &event);
	if (nret == -1)
	{
#ifdef OS_System_Windows
		closesocket(udpStruct->udpSocket);
#else 
	    close(udpStruct->udpSocket);
#endif
		udpStruct.reset();
		return e_libnet_err_epoll_ctl;
	}

	std::lock_guard<std::mutex> lock(threadLock);
 	auto ret = m_UdpStructPtrMap.insert(std::make_pair(udpStruct->srvhandle, udpStruct));

	nProcThreadOrder.fetch_add(1);

	*srvhandle = udpStruct->srvhandle ;

	//printf("svrHandle = %d, Socket = %d \r\n", UdpStruct->srvhandle, UdpStruct->svrSocket);

 	return e_libnet_err_noerror;
}

int32_t CUdpPool::DestoryUdp(NETHANDLE srvhandle)
{
	if (bRunFlag.load() == false)
		return e_libnet_err_cliopensock;

	std::lock_guard<std::mutex> lock(threadLock);
	struct epoll_event  event;

	UdpStructPtrMap::iterator it;
	int                          nSize;
	it = m_UdpStructPtrMap.find(srvhandle);
	if (it != m_UdpStructPtrMap.end())
	{
 		event.events = EPOLLIN;
		event.data.u64 = (*it).second->srvhandle ;
		epoll_ctl(epfd[(*it).second->nProcThreadOrder], EPOLL_CTL_DEL, (*it).second->udpSocket, &event);
  
  		//关闭udp的Socket套接字 
#ifdef OS_System_Windows
		closesocket((*it).second->udpSocket);
#else 
		close((*it).second->udpSocket);
#endif			
		
		m_UdpStructPtrMap.erase(it);

		return e_libnet_err_noerror;
	}
	else
		return e_libnet_err_invalidhandle;
}

UdpStructPtr CUdpPool::GetUdpPtrHandle(uint64_t nSvrHandle)
{
	std::lock_guard<std::mutex> lock(threadLock);

	UdpStructPtrMap::iterator it;
	it = m_UdpStructPtrMap.find(nSvrHandle);
	if (it != m_UdpStructPtrMap.end())
	{
 		return (*it).second ; 
  	}
	else
		return NULL ;
}

int32_t CUdpPool::Sendto(NETHANDLE udphandle,
	uint8_t* data,
	uint32_t datasize,
	void* remoteaddress)
{
	UdpStructPtr ptr = GetUdpPtrHandle(udphandle);
	if (ptr == NULL)
		return e_libnet_err_invalidhandle;
	int nSize = sizeof(sockaddr_in);
	sendto(ptr->udpSocket, (const char*)data, datasize,0,(sockaddr*)remoteaddress, nSize);
#ifndef OS_System_Windows
    usleep(10);
#endif
	return e_libnet_err_noerror;
}