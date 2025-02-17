#ifndef  _AsyncBuffer_
#define  _AsyncBuffer_

#include "auto_lock.h"

class CAsyncBuffer
{
public:
	CAsyncBuffer();
	~CAsyncBuffer();

	bool           front_pop(int nLength);
	unsigned char* pop(int& nLength);
	bool           push(unsigned char* pData, int nLength);
	bool           init(int nSize);
	bool           uninit();
	bool           reset(); 
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif

	int             nStart, nEnd;
	int             nBufferSize;
	unsigned char*  pAsyncBuffer;
};

#endif