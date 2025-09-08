/*
功能：
   实现在线程池里面对tcp客户端进行异步发送数据 

日期    2025-07-26
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "ClientSendPool.h"
#include "client_manager.h"

extern CClientReadPool*          clientReadPool ;
CClientSendPool::CClientSendPool(int nThreadCount)
{
	nThreadProcessCount.store(0);
 	nTrueNetThreadPoolCount = nThreadCount / 2 ;
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

int   CClientSendPool::GetThreadOrder()
{
	std::lock_guard<std::mutex> lock(threadLock[0]);
	int nGet = nGetCurrentThreadOrder;
	nGetCurrentThreadOrder  ++;
	return nGet;
}

CClientSendPool::~CClientSendPool()
{

}

void  CClientSendPool::destroySendPool()
{
	bRunFlag.store(false);
	int i;
	std::lock_guard<std::mutex> lock(threadLock[0]);

	for (i = 0; i < nTrueNetThreadPoolCount; i++)
	{
		notify_one(i);//通知所有线程 
		while (!bExitProcessThreadFlag[i])
			Sleep(50);
#ifdef  OS_System_Windows
		CloseHandle(hProcessHandle[i]);
#endif 
	}
}

//异步发送线程池
void CClientSendPool::ProcessFunc()
{
	int nCurrentThreadID = GetThreadOrder();
 	bExitProcessThreadFlag[nCurrentThreadID].store(false);
	int      nSendRet;//发送返回值
	int      nSize,i;
  	boost::unordered_map<NETHANDLE, client_ptr >::iterator it;
 	NETHANDLE  nClientID;
	unsigned   char* pData;
	int              nLength;
	bool             bSendErrorFlag = false;
	uint64_t         tSendTimeout = 0 ;

 	while (bRunFlag.load())
	{
		if (true)
		{//为了锁的作用域
			std::lock_guard<std::mutex> lock(threadLock[nCurrentThreadID]);
			for (it = clientMap[nCurrentThreadID].begin(); it != clientMap[nCurrentThreadID].end();  )
			{
				bSendErrorFlag = false;
				client_ptr cli = (*it).second;
				if (cli->GetConnect())
				{
 					tSendTimeout = GetTickCount64();//记录发送时间
					nSize = cli->m_circularbuff.GetSize();
 					if (cli->nPopLength == cli->nSendPos && nSize > 0 )
					{//上一次发送完成
						for (i = 0; i < nSize; i++)
						{//只要该客户端有数据尚未发送，就一次性全部发送完毕 ，减少发送次数 
							cli->pPopData = cli->m_circularbuff.pop(&cli->nPopLength);
							if (cli->pPopData && cli->GetConnect())
							{
								cli->nSendPos = 0;
								cli->nSendLength = cli->nPopLength;

								while (cli->nSendLength > 0 && cli->GetConnect())
								{
									nSendRet = send(cli->m_Socket, (char*)cli->pPopData + cli->nSendPos, cli->nSendLength, 0);

									if (nSendRet > 0)
									{//发送成功 
										cli->nSendPos += nSendRet;
										cli->nSendLength -= nSendRet;

										//发送完毕,执行删除 
										if (cli->nSendPos == cli->nPopLength)
											cli->m_circularbuff.pop_front();
										else
										{//尚未发送完毕 
										}
									}
#ifdef OS_System_Windows
				                   else if (nSendRet == 0 || ((GetTickCount64() - tSendTimeout) >= 1000 * 2) || (nSendRet == SOCKET_ERROR && (WSAGetLastError() != 10035 && WSAGetLastError() != EWOULDBLOCK && WSAGetLastError() != EAGAIN)))
#else 
				                   else if (nSendRet == 0 || ((GetTickCount64() - tSendTimeout) >= 1000 * 2) || (nSendRet == -1 && (errno != EWOULDBLOCK && errno != EAGAIN)))
#endif									
									{//断线、发送超时达2秒、或者错误 
									    //printf("------------------ 发送出错, nClient = %llu ，nSendRet = %d, WSAGetLastError() = %d ---------------\r\n", cli->get_id(), nSendRet, WSAGetLastError());
  									    nSize = 0;
 									    bSendErrorFlag = true;
									    cli->m_circularbuff.Reset();
   										break;
									}
								}//while (cli->nSendLength > 0)
							}
							else//if (cli->pPopData)
							{
								nSize = 0;
								cli->m_circularbuff.Reset(); 
							}
						}//for (int i = 0; i < nSize; i++)
 					}//if (cli->nPopLength == cli->nSendPos && nSize > 0)
				}
				else
				{//断开连接 
					bSendErrorFlag = true;
				}

				if (bSendErrorFlag == false)
					it ++;
				else
				{//需要删除
					clientReadPool->DeleteFromTask(cli->get_id());//从读取线程中移除
					client_manager_singleton::get_mutable_instance().pop_client(cli->get_id());

					if (cli->m_fnclose)
						cli->m_fnclose(cli->get_server_id(), cli->get_id());

					clientMap[nCurrentThreadID].erase(it++);
 				}
  			}
		}

		if (bRunFlag)
		{
 			std::unique_lock<std::mutex> lck(mtx[nCurrentThreadID]);
			cv[nCurrentThreadID].wait(lck);
 		}
		else
			break;
	}
 
	bExitProcessThreadFlag[nCurrentThreadID].store(true);
}

void* CClientSendPool::OnProcessThread(void* lpVoid)
{
	int nRet = 0 ;
#ifndef OS_System_Windows
	pthread_detach(pthread_self()); //让子线程和主线程分离，当子线程退出时，自动释放子线程内存
#endif

	CClientSendPool* pThread = (CClientSendPool*)lpVoid;
	pThread->ProcessFunc();

#ifndef OS_System_Windows
	pthread_exit((void*)&nRet); //退出线程
#endif
	return  0;
}

//获取发送线程序号
uint32_t  CClientSendPool::GetSendOrder()
{
	std::lock_guard<std::mutex> lock(getOrderMutex);

	int    nThreadThread = nThreadProcessCount.load() % nTrueNetThreadPoolCount;
 	nThreadProcessCount.add(1);

	return nThreadThread;
}

bool CClientSendPool::InsertIntoTask(uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;
	client_ptr cli =  client_manager_singleton::get_mutable_instance().get_client(nClientID);
	if (cli == NULL)
		return false ;

  	int                 nThreadThread = GetSendOrder() ;
    std::lock_guard<std::mutex> lock(threadLock[nThreadThread]);
	int                 ret;
 
	cli->nSendThreadOrder.store(nThreadThread);
	clientMap[nThreadThread].insert(std::make_pair(nClientID, cli));
 
	return true ;
}

//从线程池彻底移除 nClient 
bool CClientSendPool::DeleteFromTask(int nThreadOrder, uint64_t nClientID)
{
	if (bRunFlag.load() == false)
		return false;
	std::lock_guard<std::mutex> lock(threadLock[nThreadOrder]);
	if (nThreadOrder >= MaxNetHandleQueueCount)
		return false;

	boost::unordered_map<NETHANDLE, client_ptr >::iterator it;
	it = clientMap[nThreadOrder].find(nClientID);
	if(it != clientMap[nThreadOrder].end())
	{
		clientMap[nThreadOrder].erase(it);
		return true;
	}

    return false  ;
}

bool  CClientSendPool::notify_one(int nThreadOrder)
{
	if (nThreadOrder >= MaxNetHandleQueueCount)
		return false;

	cv[nThreadOrder].notify_one();
	return true;
}
