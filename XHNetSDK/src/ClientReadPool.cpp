/*
���ܣ�
  ʵ�ֶ��Ѿ��ɹ����ӵ�tcp�ͻ��˽��ж�ȡ���������ݣ�֧��ͬ�� ���첽���ͣ�ֻ֧���첽��ȡ 

����    2025-07-09
����    �޼��ֵ�
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "ClientReadPool.h"
#include "client_manager.h"
#include "ClientSendPool.h"

extern CClientSendPool*          clientSendPool;

CClientReadPool::CClientReadPool(int nThreadCount)
{
	nThreadProcessCount = 0;
 	nTrueNetThreadPoolCount = nThreadCount / 2;
	if (nThreadCount > MaxNetHandleQueueCount)
		nTrueNetThreadPoolCount = MaxNetHandleQueueCount;

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

int   CClientReadPool::GetThreadOrder()
{
	std::lock_guard<std::mutex> lock(threadLock);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder  ++;
	return nGet;
}

CClientReadPool::~CClientReadPool()
{

}

void  CClientReadPool::destroyReadPool()
{
	bRunFlag.store(false);
	int i;
	std::lock_guard<std::mutex> lock(threadLock);

	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{
#ifdef OS_System_Windows		
		epoll_close(epfd[i]);
#else
		close(epfd[i]);
#endif
 		while (!bExitProcessThreadFlag[i].load())
			Sleep(50);

#ifdef  OS_System_Windows
		CloseHandle(hProcessHandle[i]);
#endif 
	}
}

void CClientReadPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
 	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	uint64_t nClientID = 0 ;
	int      ret_num;
	int      i;
	int      ret_recv;
	unsigned char*    szRecvData;
 
	//����epoll���epfd
	epfd[nCurrentThreadID] = epoll_create(1);
	szRecvData = new unsigned char[1024*1024*2];

	while (bRunFlag.load())
	{
		ret_num = epoll_wait(epfd[nCurrentThreadID], events[nCurrentThreadID], MaxEventCount, 1000);
		if (ret_num > 0)
		{
			for (i = 0; i < ret_num; i++)//�����������¼�����
			{
				client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(events[nCurrentThreadID][i].data.u64);
				if (cli == NULL)
					continue;

				ret_recv = ::recv(cli->m_Socket, (char*)szRecvData, 1024 * 1024 * 2 , 0);//��������
				if (ret_recv > 0)//���ݳɹ����մ�ӡ
				{
					if(ret_recv < 1024*1024*2)
					  szRecvData[ret_recv] = 0x00;

					if (cli->m_fnread)
						cli->m_fnread(0, events[nCurrentThreadID][i].data.u64, szRecvData, static_cast<uint32_t>(ret_recv), (void*)&cli->tAddr4);
				}
				else
				{//�ͻ��˶Ͽ�����
#ifdef OS_System_Windows
					if (ret_recv == 0 || (ret_recv == SOCKET_ERROR && (WSAGetLastError() != EWOULDBLOCK && WSAGetLastError() != EAGAIN)))
#else 
					if (ret_recv == 0 || (ret_recv == -1 && (errno != EWOULDBLOCK && errno != EAGAIN)))
#endif
					{//���ӶϿ� 
						clientSendPool->DeleteFromTask(cli->nSendThreadOrder.load(), cli->get_id());
 						DeleteFromTask(events[nCurrentThreadID][i].data.u64);//�Ӷ�ȡ�߳����Ƴ�
						client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());

 						if(cli->m_fnclose)
							cli->m_fnclose(cli->get_server_id(), cli->get_id());					
    				}

					continue ;//����ִ����һ��
				}
			}
		}
		else
			Sleep(2);
  	}
	delete[] szRecvData;
	szRecvData = NULL;
	
	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CClientReadPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //�����̺߳����̷߳��룬�����߳��˳�ʱ���Զ��ͷ����߳��ڴ�
#endif

	CClientReadPool* pThread = (CClientReadPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //�˳��߳�
#endif
	return  0;
}

bool CClientReadPool::InsertIntoTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;
    std::lock_guard<std::mutex> lock(threadLock);
	int                 nThreadThread = 0;
	int                 ret;
	static  int nAddCount = 0;

	nThreadThread = nThreadProcessCount.load() % nTrueNetThreadPoolCount;
 	nThreadProcessCount.add(1);
 
	client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(nClientID);
	if (cli)
	{
		struct epoll_event  event;//��event�ṹ������������
		event.events  = EPOLLIN;
	    event.data.u64 = cli->get_id();
		cli->nRecvThreadOrder.store(nThreadThread);
		ret = epoll_ctl(epfd[nThreadThread], EPOLL_CTL_ADD, cli->m_Socket, &event);
		if (ret == -1)
		{
			perror("epoll_ctl cfd");
			return false ;
		}

		nAddCount++;
		//printf("=====================================  nAddCount = %d\r\n", nAddCount);
		return true;
	}

	return false ;
}

//���̳߳س����Ƴ� nClient 
bool CClientReadPool::DeleteFromTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;

	bool       bRet = false;
	struct epoll_event  event;//��event�ṹ������������

	client_ptr cli = client_manager_singleton::get_mutable_instance().get_client(nClientID);

	if( cli != NULL)
	{
		event.events  = EPOLLIN;
		event.data.u64 = cli->get_id();
		epoll_ctl(epfd[cli->nRecvThreadOrder.load()], EPOLL_CTL_DEL, cli->m_Socket, &event);

  		bRet = true;
	}
	else
		bRet = false;

    return bRet;
}
