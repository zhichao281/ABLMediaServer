/*
  ���ܣ�
     ʵ�ִ洢�����������ݿ飬Ŀ��ֻ����һ�Σ��ṩ���ⲿʹ�ã��ⲿֱ��ʹ���ڲ��ڴ�
	 �ⲿʹ���ڲ��ڴ�ʱ�����Ը������ݣ����ǲ���ɾ���ڴ�� 
  ����    2021-03-21
  ����    �޼��ֵ�
  QQ     79941308
  E-Mail 79941308@qq.com 
*/

#include "stdafx.h"
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
	malloc_trim(0);
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

	//��δ�����ڴ� 
	if (nMediaBufferLength <= 0 || fifoBuffer == NULL || pMediaBuffer == NULL || nLength <= 0 || nLength > nMediaBufferLength)
		return false;

	int  nStart = 0, nEnd = 0;//��¼�������ĳ��� 
	MediaNode node,node2;

	if (LengthList.size() <= 0)
		nEnd = 0;
	else
	{
		node2 = LengthList.front();
		nStart = node2.nBegin;  //��ʼ

		node = LengthList.back();
		nEnd = node.nEnd; //���
	}

	//���ʣ��Ŀռ��Ƿ񹻴洢 
	if (nMediaBufferLength - nFifoEnd < nLength)
	{//ʣ��ռ䲻������ͷ��ʼ�洢 
		nFifoEnd = 0;
	}

	if (LengthList.size() > 0)
	{//
		if (nFifoEnd == 0)
		{//��ͷ��ʼ
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
				{//�ռ䲻��
					return false;
				}
 			}else
			{
				if (nMediaBufferLength - nFifoEnd < nLength)
				{//ʣ��ռ䲻��
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

//��ȡ��ֱ�ӷ����ڲ��ڴ�飬���ⲿʹ�ã��ⲿ����ɾ�����ڴ�飬
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
	return NULL;
}

//����ɾ��,ʹ�������ִ�У������ڴ�ò������� 
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

//��ȡʣ���ڴ�ռ� 
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

//��ȡFIFO �ڴ��С�ĳ���
int   CMediaFifo::GetFifoLength()
{
	std::lock_guard<std::mutex> lock(MediaFifoMutex);
	return  nMediaBufferLength;
}
