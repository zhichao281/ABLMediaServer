/*
功能：
   实现视频解码 、JPEG图片抓拍

日期    2022-03-17
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "FFVideoDecode.h"

extern MediaServerPort     ABL_MediaServerPort;

CFFVideoDecode::CFFVideoDecode()
{
   m_bInitDecode = false ;
   pDCodecCtx = NULL ;
   memset(m_szCodecName, 0x00, sizeof(m_szCodecName));
   m_nWidth = m_nHeight = 0;
   nFrameGopCount = 0;
   pDPicture = NULL;
   m_nWidth = m_nHeight = 0;
   frameSWS = NULL;
   WriteLog(Log_Debug, "CFFVideoDecode 构造 = %X line= %d ", this, __LINE__);
}

CFFVideoDecode::~CFFVideoDecode()
{
  stopDecode() ;
  malloc_trim(0);
  WriteLog(Log_Debug, "CFFVideoDecode 析构 = %X line= %d ", this, __LINE__);
}

//启动解码器
bool CFFVideoDecode::startDecode(char* szCodecName, int nWidth, int nHeight)
{
	std::lock_guard<std::mutex> lock(m_decodeLockMutex);

	if (m_bInitDecode || !(strcmp(szCodecName,"H264") == 0 || strcmp(szCodecName,"H265") == 0) )
	{
		return false ;
	}
	int nRet;

	strcpy(m_szCodecName, szCodecName);

 	if (strcmp(szCodecName, "H264") == 0)
		pDCodec = avcodec_find_decoder_by_name("h264");
	else if (strcmp(szCodecName, "H265") == 0)
		pDCodec = avcodec_find_decoder_by_name("hevc");

	packet = av_packet_alloc() ;
	pDCodecCtx = avcodec_alloc_context3(pDCodec);
	pDPicture = av_frame_alloc();
	//pDPicture->format = AV_PIX_FMT_YUVJ420P;

    pDCodecCtx->slice_count = 4;
	pDCodecCtx->thread_count = 4;
 
	if (avcodec_open2(pDCodecCtx, pDCodec, NULL) < 0)
	{
		WriteLog(Log_Debug, "FFVideoDecode = %X 创建解码器失败 line= %d,", this, __LINE__);
		return false  ;
	}
	nDecodeErrorCount = 0;

	m_bInitDecode = true;
#ifdef  WriteFFVideoDecodeYUVFlag
	  nWriteYUVCount = 0;
	  sprintf(yuvFileName,"%sDecodeYUV.yuv", ABL_MediaServerPort.recordPath);
	  fWriteYUV = fopen(yuvFileName, "wb");
#endif

	WriteLog(Log_Debug, "FFVideoDecode = %X 创建解码器成功 line= %d ", this, __LINE__);
	return true;
}

//停止解码器
void CFFVideoDecode::stopDecode()
{
   std::lock_guard<std::mutex> lock(m_decodeLockMutex);
   if (m_bInitDecode)
  {
	 m_bInitDecode = false  ;
	 m_nWidth = m_nHeight = 0; 
 	 WriteLog(Log_Debug, "FFVideoDecode = %X 停止解码 line= %d  this= %X Step1 ", this, __LINE__, this);
  	 av_frame_free(&pDPicture);
	 WriteLog(Log_Debug, "FFVideoDecode = %X 停止解码 line= %d  this= %X Step2 ", this, __LINE__, this);
 	 av_packet_unref(packet);
	 av_packet_free(&packet);
	 WriteLog(Log_Debug, "FFVideoDecode = %X 停止解码 line= %d  this= %X Step3", this, __LINE__, this);
	 avcodec_close(pDCodecCtx);
  	 avcodec_free_context(&pDCodecCtx);
 	 WriteLog(Log_Debug, "FFVideoDecode = %X 停止解码 line= %d  this= %X Step4 ", this, __LINE__, this);
   }
   if (frameSWS)
   {
	   frameSWS->DeleteAVFrameSws();
	   SAFE_DELETE(frameSWS);
   }
#ifdef  WriteFFVideoDecodeYUVFlag
   if (fWriteYUV)
   {
     fclose(fWriteYUV);
	 fWriteYUV = NULL;
   }
#endif
}

//解码
int CFFVideoDecode::DecodeYV12Image(unsigned char *szImage, int nImageLen)
{
	std::lock_guard<std::mutex> lock(m_decodeLockMutex);
	nRet = -1;
	if (!m_bInitDecode || szImage == NULL || nImageLen <= 0 )
	{
		return -1;
	}
	try
	{
 		packet->data = szImage;
		packet->size = nImageLen;

		nGet = avcodec_send_packet(pDCodecCtx, packet);
		if (nGet < 0)
		{
			av_packet_unref(packet);
			return -1;
		}

		nGet = avcodec_receive_frame(pDCodecCtx, pDPicture);
		if (nGet == 0)
			nGet = nImageLen ;
		else if (nGet == AVERROR(EAGAIN) || nGet == AVERROR_EOF || nGet < 0)
		{
			av_packet_unref(packet);
			return -1;
		}
 
		if (pDPicture->width > 0 && pDPicture->height >0 )
		{//记录视频宽，高,如宽、高有变化，需要改变 
			if (m_nWidth != pDPicture->width )
				m_nWidth = pDPicture->width ;
			if (m_nHeight != pDPicture->height)
				m_nHeight = pDPicture->height;
		}
		av_packet_unref(packet);
 
		if(nGet > 0)
		{//nGet > 0 解码成功
			nDecodeErrorCount = 0;
		}
		else
		{//解码失败
			nDecodeErrorCount++;
			nGet = -1;
		}

	}catch(...)
	{
		nDecodeErrorCount ++ ;  //解码出错
		nGet = -1 ;
	}

	if (nGet > 0)
	{
		if (pDPicture->width > 0 && pDPicture->height > 0 && pDPicture->data[0] != NULL)
		{
#ifdef  WriteFFVideoDecodeYUVFlag
			if (fWriteYUV)
			{
				fwrite(pDPicture->data[0], 1, pDPicture->width * pDPicture->height * 3 / 2, fWriteYUV);
				fflush(fWriteYUV) ;
			}
#endif
			return nGet;
		}
		else
			return -1;
	}else 
       return nGet;
}
