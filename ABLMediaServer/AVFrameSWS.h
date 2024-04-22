#ifndef _CAVFrameSWS_H
#define _CAVFrameSWS_H

class CAVFrameSWS 
{
public:
	CAVFrameSWS(void);
	virtual ~CAVFrameSWS(void);

#ifdef WriteAVFrameSWSYUVFlag
	FILE* fWriteYUV;
	char  szYUVName[256];
	int   nWriteYUVCount;
#endif

	bool CreateAVFrameSws(bool bCreeateSrcFrame, AVPixelFormat nSrcImage, int nSrcWidth, int nSrcHeight, AVPixelFormat nDstImage, int nDstWidth, int nDestHeight, int swsType);
	bool AVFrameSWS(AVFrame* pPicture);
	bool AVFrameSWSYUV(unsigned char* pYUV,int nLength);
	bool DeleteAVFrameSws();
	bool GetStatus();

	bool             m_bCreeateSrcFrame;
	int              numBytes1,numBytes2;
	bool             bInitFlag;
	std::mutex       swsLock;
	int              m_nWidth, m_nHeight,m_nWidthDst,m_nHeightDst ;
	unsigned char*   szSrcData;
	AVFrame*         pFrameSrc;
	unsigned char*   szDestData;
	AVFrame*         pFrameDest;
	SwsContext *     img_convert_ctx; 
};

#endif 
