/*
  功能：
     实现存储不定长的数据块，目的只拷贝一次，提供给外部使用，外部直接使用内部内存
	 外部使用内部内存时，可以更改数据，但是不能删除内存块 
*/


#include "MediaFifo.h"

CMediaFifo::CMediaFifo()
{
	pMediaBuffer = NULL;
	nMediaBufferLength = 0;
	nFifoStart = 0;
	nFifoEnd = 0 ;
}

CMediaFifo::~CMediaFifo()
{
	FreeFifo();

}

size_t CMediaFifo::GetSize()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	
	return LengthList.size() ;
}

void CMediaFifo::Reset()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);

	nFifoStart = 0;
	nFifoEnd = 0;
	LengthList.clear();
}

void  CMediaFifo::InitFifo(int nBufferLength)
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	if (pMediaBuffer != NULL)
		return; 
	nFifoStart = 0;
	nFifoEnd = 0;
	LengthList.clear();
	while(pMediaBuffer == NULL)
	  pMediaBuffer = new unsigned char[nBufferLength];
	nMediaBufferLength = nBufferLength;
	memset(pMediaBuffer, 0x00, nMediaBufferLength);
}

void  CMediaFifo::FreeFifo()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	nFifoStart = 0;
	nFifoEnd = 0;
	LengthList.clear();
	if (pMediaBuffer != NULL)
	{
		delete [] pMediaBuffer;
		pMediaBuffer = NULL;
		nMediaBufferLength = 0;
	}
}

bool  CMediaFifo::push(unsigned char* fifoBuffer, int nLength)
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);

	//尚未分配内存 
	if (nMediaBufferLength <= 0 || fifoBuffer == NULL || pMediaBuffer == NULL || nLength <= 0 || nLength > nMediaBufferLength)
		return false;

	int  nStart = 0, nEnd = 0;//记录链表最后的长度 
	MediaNode node,node2;

	if (LengthList.size() <= 0)
		nEnd = 0;
	else
	{
		node2 = LengthList.front();
		nStart = node2.nBegin;  //开始

		node = LengthList.back();
		nEnd = node.nEnd; //最后
	}

	//检测剩余的空间是否够存储 
	if (nMediaBufferLength - nFifoEnd < nLength)
	{//剩余空间不够，重头开始存储 
		nFifoEnd = 0;
	}

	if (LengthList.size() > 0)
	{//
		if (nFifoEnd == 0)
		{//重头开始
			if (nStart < nLength)
			{
				return false;
			}
		}
		else
		{
			if (nStart >= nFifoEnd)
			{
				if ((nStart - nFifoEnd) < nLength)
				{//空间不够
					return false;
				}
 			}else
			{
				if (nMediaBufferLength - nFifoEnd < nLength)
				{//剩余空间不够
					return false;
				}
			}
		}
	}

	node.nBegin = nFifoEnd;
	node.nEnd = node.nBegin + nLength;
	LengthList.push_back(node);
	memcpy(pMediaBuffer + nFifoEnd, fifoBuffer, nLength);
	nFifoEnd += nLength;

	return true;
}

//获取，直接返回内部内存块，供外部使用，外部不能删除该内存块，
unsigned char*  CMediaFifo::pop(int* nLength)
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	unsigned char* fifoBuffer = NULL ;
	if (LengthList.size() <= 0)
	{
		fifoBuffer = NULL;
		*nLength = 0;
		return NULL ;
	}
	MediaNode node;
	node = LengthList.front();
	if (node.nBegin >= 0 && 
		node.nEnd > 0 && 
		node.nBegin < nMediaBufferLength &&
		node.nEnd <= nMediaBufferLength && 
		(node.nEnd - node.nBegin > 0) && 
		(node.nEnd - node.nBegin <= nMediaBufferLength))
	{
		fifoBuffer = pMediaBuffer + (node.nBegin);
		*nLength = node.nEnd - node.nBegin;
		return fifoBuffer ;
	}
	else
	{
		fifoBuffer = NULL;
		*nLength = 0;
		return fifoBuffer;
	}
}

//真正删除,使用完毕再执行，否则内存得不到保护 
bool  CMediaFifo::pop_front()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);

	if (LengthList.size() <= 0)
		return false;
	LengthList.pop_front();
	if (LengthList.size() <= 0)
	{
		nFifoStart = 0;
		nFifoEnd = 0;
	}
	return true;
}

//获取剩余内存空间 
int  CMediaFifo::GetFreeSpaceByte()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);

	if (nMediaBufferLength <= 0)
		return nMediaBufferLength;

	if (LengthList.size() <= 0)
		return  nMediaBufferLength;

	MediaFifoLengthList::iterator it;
	MediaNode node;
	int       nListLengthCount = 0;
	for (it = LengthList.begin(); it != LengthList.end(); ++it)
	{
		node = *it;
 		nListLengthCount += node.nEnd - node.nBegin;
	}
  
	return  nMediaBufferLength - nListLengthCount ;
}   

//获取FIFO 内存大小的长度
int   CMediaFifo::GetFifoLength()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	return  nMediaBufferLength;
}
