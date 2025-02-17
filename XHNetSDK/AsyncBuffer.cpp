#include <stdio.h>
#include <string.h>
#include "AsyncBuffer.h"
#include <malloc.h>

CAsyncBuffer::CAsyncBuffer()
{
	 nStart = nEnd = 0;
	 pAsyncBuffer = NULL ;
}

CAsyncBuffer::~CAsyncBuffer()
{
	uninit();
#ifndef _WIN32
	malloc_trim(0);
#endif
}

//初始化
bool CAsyncBuffer::init(int nSize)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (pAsyncBuffer)
		return false;
	while (pAsyncBuffer == NULL)
		pAsyncBuffer = new unsigned char[nSize];
	nStart = nEnd = 0; 
	nBufferSize = nSize;

	return true;
}

//销毁内存
bool  CAsyncBuffer::uninit()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (pAsyncBuffer)
	{
		delete [] pAsyncBuffer;
		pAsyncBuffer = NULL;

	    nStart = nEnd = 0 ; 
	    return true;
	}
	else
		return false;
}

//加入待发送buffer
bool CAsyncBuffer::push(unsigned char* pData, int nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (nLength > nBufferSize || pAsyncBuffer == NULL || nLength > (nBufferSize - (nEnd - nStart)) )
		return false;

	if (nBufferSize - nEnd < nLength  )
	{//尾部空间不够存储，把剩余数据往前移动 
		memmove(pAsyncBuffer, pAsyncBuffer + nStart, nEnd - nStart);
		nStart = 0;
		nEnd = nEnd - nStart;
	}
 
	memcpy(pAsyncBuffer + nEnd, pData, nLength);
	nEnd += nLength;

	return true;
}

//取出buffer进行发送
unsigned char*  CAsyncBuffer::pop(int& nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif
	if (nEnd - nStart <= 0)
	{
		nLength = 0;
		return NULL;
	}

	nLength = nEnd - nStart;
	return  pAsyncBuffer + nStart ;
}

//进行移除已经发送的数据
bool  CAsyncBuffer::front_pop(int nLength)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (nLength > nBufferSize)
		return false;

	nStart += nLength;

	if (nStart == nEnd)
		nStart = nEnd = 0;

	return true;
}

//清空数据
bool   CAsyncBuffer::reset()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	nStart = nEnd = 0; 

	return true;
}
