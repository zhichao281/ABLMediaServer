/*
功能： Frame转换器

Author 罗家兄弟
Date   2017-12-16
QQ     79941308
E-mail luoshizhen2003@gmail.com

*/

#include "stdafx.h"
#include "AVFrameSWS.h"

CAVFrameSWS::CAVFrameSWS(void)
{
	m_bCreeateSrcFrame = false;
	bInitFlag = false;
	szDestData = NULL;
	pFrameDest = NULL;
	szSrcData = NULL ;
	pFrameSrc = NULL ;
 	img_convert_ctx = NULL; //转换器
#ifdef WriteAVFrameSWSYUVFlag
	fWriteYUV = NULL ;
	nWriteYUVCount = 0;
#endif
}

CAVFrameSWS::~CAVFrameSWS(void)
{
	DeleteAVFrameSws();
	malloc_trim(0);
}

/*
功能： 创建转换器
参数：
    bool          bCreeateSrcFrame  是否需要创建原始frame 
    AVPixelFormat nSrcImage,     原始像素
	int           nSrcWidth,     原始宽
	int           nSrcHeight,    原始高
	AVPixelFormat nDstImage,     目标像素
	int           nDstWidth,     目标宽
	int           nDestHeight,   目标高
	int           swsType        转换缩放 算法
*/
bool CAVFrameSWS::CreateAVFrameSws(bool bCreeateSrcFrame,AVPixelFormat nSrcImage, int nSrcWidth, int nSrcHeight, AVPixelFormat nDstImage, int nDstWidth, int nDestHeight, int swsType)
{
	std::lock_guard<std::mutex> lock(swsLock);
	if (bInitFlag)
	{
		return true;
	}

	m_nWidth     = nSrcWidth;
	m_nHeight    = nSrcHeight;
	m_nWidthDst  = nDstWidth;
	m_nHeightDst = nDestHeight ;
	m_bCreeateSrcFrame = bCreeateSrcFrame;

	if (m_bCreeateSrcFrame)
	{//cuda需要这种方式 
		numBytes1 = av_image_get_buffer_size((enum AVPixelFormat)nSrcImage, m_nWidth, m_nHeight, 1);
		pFrameSrc = av_frame_alloc();
		szSrcData = new unsigned char[numBytes1];
		av_image_fill_arrays(pFrameSrc->data, pFrameSrc->linesize, szSrcData, nSrcImage, m_nWidth, m_nHeight, 1);
	}

	numBytes2 = av_image_get_buffer_size((enum AVPixelFormat)nDstImage, nDstWidth, nDestHeight,1);
	pFrameDest = av_frame_alloc();
 	szDestData = new unsigned char[numBytes2];
	av_image_fill_arrays(pFrameDest->data, pFrameDest->linesize, szDestData, nDstImage, nDstWidth, nDestHeight, 1);

	img_convert_ctx = sws_getContext(nSrcWidth, nSrcHeight,  //长、宽（原始的）
		nSrcImage,                //原像素格式
		nDstWidth, nDestHeight,    //长、宽 (目标的)
		(enum AVPixelFormat)nDstImage, //目标像素
		swsType, NULL, NULL, NULL);//SWS_BICUBIC

	pFrameDest->width = nDstWidth;
	pFrameDest->height = nDestHeight;
	pFrameDest->format = nDstImage ;//指定为输出格式

	bInitFlag = true;
	return true;
}

bool CAVFrameSWS::AVFrameSWS(AVFrame* pPicture)
{
	std::lock_guard<std::mutex> lock(swsLock);
	bool bRet;
	int  nRet;
	if (bInitFlag && img_convert_ctx != NULL)
	{
		nRet = sws_scale(img_convert_ctx, pPicture->data, pPicture->linesize, 0, m_nHeight, pFrameDest->data, pFrameDest->linesize);
		bRet = true;
	}
	else
		bRet = false;
	return bRet;
}

bool CAVFrameSWS::AVFrameSWSYUV(unsigned char* pYUV, int nLength)
{
	std::lock_guard<std::mutex> lock(swsLock);
	if (!m_bCreeateSrcFrame || pYUV == NULL || nLength <= 0 || nLength > ((m_nWidth * m_nHeight * 3) / 2) )
		return false;

	bool bRet;
	int  nRet;
 
	if (bInitFlag && img_convert_ctx != NULL)
	{
		memcpy(szSrcData, pYUV, nLength);
		nRet = sws_scale(img_convert_ctx, pFrameSrc->data, pFrameSrc->linesize, 0, m_nHeight, pFrameDest->data, pFrameDest->linesize);
		bRet = true;
	}
	else
		bRet = false;
	return bRet;
}

bool CAVFrameSWS::DeleteAVFrameSws()
{
	std::lock_guard<std::mutex> lock(swsLock);
	bool bRet;
	if (bInitFlag)
	{
		if (pFrameDest != NULL)
		{
			av_frame_free(&pFrameDest);//avcodec_alloc_frame avcodec_free_frame
			pFrameDest = NULL;
		}
		if (img_convert_ctx != NULL)
		{
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
		}
		SAFE_ARRAY_DELETE(szDestData);

		if (m_bCreeateSrcFrame)
		{//支持cuda
			if (pFrameSrc != NULL)
			{
				av_frame_free(&pFrameSrc); 
				pFrameSrc = NULL;
			}
			SAFE_ARRAY_DELETE(szSrcData);
		}

		bInitFlag = false;
		bRet = true;
	}
	else
		bRet = false;

#ifdef WriteAVFrameSWSYUVFlag
	if (fWriteYUV != NULL)
	{
		fclose(fWriteYUV);
		fWriteYUV = NULL;
	}
#endif
	return bRet;
}

bool CAVFrameSWS::GetStatus()
{
	std::lock_guard<std::mutex> lock(swsLock);
	return bInitFlag;
}
