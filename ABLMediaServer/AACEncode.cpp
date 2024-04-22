/* 
功能：    实现音频数据的AAC编码 
Author    罗家兄弟 
QQ        79941308
E-mail    79941308@qq.com 
Date      2022-02-25
*/

#include "stdafx.h"
#include "AACEncode.h"

CAACEncode::CAACEncode()
{
	hEncoder = NULL;
}

CAACEncode::~CAACEncode()
{
	ExitAACEncodec();
	malloc_trim(0);
}

bool CAACEncode::InitAACEncodec(int bit_rate1, int Sample1, int nChan1, int *nSampleLen)
{
	if (hEncoder != NULL)
		return true;

	nInputSamples = 0;
	// (1) Open FAAC engine
	hEncoder = faacEncOpen(Sample1, nChan1, &nInputSamples, &nMaxOutputBytes);
	if (hEncoder == NULL)
	{
		printf("[ERROR] Failed to call faacEncOpen()\n");
		return -1;
	}
	nPCMBitSize = 16;      // 单样本位数

	nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
	pbPCMBuffer = new unsigned char[nPCMBufferSize];
	pbAACBuffer = new unsigned char[nMaxOutputBytes];

	// (2.1) Get current encoding configuration
	pConfiguration = faacEncGetCurrentConfiguration(hEncoder);

	pConfiguration->inputFormat = FAAC_INPUT_16BIT;

	// (2.2) Set encoding configuration
	int nRet = faacEncSetConfiguration(hEncoder, pConfiguration);

	*nSampleLen = 2048;

	return true ;
}

/*
 参数
        short    *sample1,   待编码的数据
	    int      inSize,     待编码数据的长度
	    uint8_t  *szOut      编码后的缓冲

 返回值
        返回整型，是编码后的数据长度
*/
int CAACEncode::EncodecAAC(int* nLength)
{
	if (hEncoder != NULL)
	{
		// 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
		nInputSamples = nPCMBufferSize / (nPCMBitSize / 8);
		// (3) Encode
		int nRet = faacEncEncode(
			hEncoder, (int*)pbPCMBuffer, nInputSamples, pbAACBuffer, nMaxOutputBytes);

		*nLength = nRet;

		return nRet;
	}
	else
		return -1;
}

void CAACEncode::ExitAACEncodec()
{
	if (hEncoder != NULL)
	{
	  int nRet = faacEncClose(hEncoder);

	  delete [] pbPCMBuffer;
	  delete [] pbAACBuffer;
	  hEncoder = NULL;
	}
}
