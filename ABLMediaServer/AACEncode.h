
#ifndef _AACEncode_H
#define _AACEncode_H

#include "faac.h"

class CAACEncode  
{
public:
	void ExitAACEncodec();
	int  EncodecAAC(int* nLength);
	bool InitAACEncodec(int bit_rate1, int Sample1, int nChan1, int* nSampleLen);
	
	CAACEncode();
	virtual ~CAACEncode();

	faacEncHandle hEncoder;
	faacEncConfigurationPtr pConfiguration;

	unsigned long  nInputSamples ;
	uint32_t nPCMBitSize;
	int nBytesRead;
	int nPCMBufferSize;
	unsigned char* pbPCMBuffer;
	unsigned char* pbAACBuffer;
	unsigned long  nMaxOutputBytes;

};

#endif   
