/*
功能：
   实现视频解码 、JPEG图片抓拍

日期    2022-03-17
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "VideoDecode.h"

extern MediaServerPort     ABL_MediaServerPort;

CVideoDecode::CVideoDecode()
{
   m_bInitDecode = false ;
   pDCodecCtx = NULL ;
   memset(m_szCodecName, 0x00, sizeof(m_szCodecName));
   m_nWidth = m_nHeight = 0;
   nFrameGopCount = 0;
   pDPicture = NULL;
   frameSWS = NULL;
   WriteLog(Log_Debug, "CVideoDecode 构造 = %X line= %d ", this, __LINE__);
}

CVideoDecode::~CVideoDecode()
{
  stopDecode() ;
  malloc_trim(0);
  WriteLog(Log_Debug, "CVideoDecode 析构 = %X line= %d ", this, __LINE__);
}

//启动解码器
bool CVideoDecode::startDecode(char* szCodecName, int nWidth, int nHeight)
{
	std::lock_guard<std::mutex> lock(m_decodeLockMutex);

	if (m_bInitDecode || !(strcmp(szCodecName,"H264") == 0 || strcmp(szCodecName,"H265") == 0) )
	{
		return false ;
	}
	int nRet;

	strcpy(m_szCodecName, szCodecName);

 	if (strcmp(szCodecName, "H264") == 0)
		pDCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	else if (strcmp(szCodecName, "H265") == 0)
		pDCodec = avcodec_find_decoder(AV_CODEC_ID_H265);

	packet = av_packet_alloc();
	pDCodecCtx = avcodec_alloc_context3(pDCodec);
	pDPicture = av_frame_alloc();
 
 	pDCodecCtx->slice_count = 4;
	pDCodecCtx->thread_count = 4;

	if (avcodec_open2(pDCodecCtx, pDCodec, NULL) < 0)
	{
		WriteLog(Log_Debug, "CVideoDecode = %X 创建解码器失败 line= %d,", this, __LINE__);
		return false  ;
	}
	nDecodeErrorCount = 0;

	m_bInitDecode = true;

	WriteLog(Log_Debug, "CVideoDecode = %X 创建解码器成功 line= %d ", this, __LINE__);
	return true;
}

//停止解码器
void CVideoDecode::stopDecode()
{
   std::lock_guard<std::mutex> lock(m_decodeLockMutex);
   if (m_bInitDecode)
  {
	 m_bInitDecode = false  ;
	 m_nWidth = m_nHeight = 0; 
 	 WriteLog(Log_Debug, "CVideoDecode = %X 停止解码 line= %d  this= %X Step1 ", this, __LINE__, this);
  	 av_frame_free(&pDPicture);
	 WriteLog(Log_Debug, "CVideoDecode = %X 停止解码 line= %d  this= %X Step2 ", this, __LINE__, this);
 	 av_packet_unref(packet);
	 av_packet_free(&packet);
	 WriteLog(Log_Debug, "CVideoDecode = %X 停止解码 line= %d  this= %X Step3", this, __LINE__, this);
	 avcodec_close(pDCodecCtx);
  	 avcodec_free_context(&pDCodecCtx);
 	 WriteLog(Log_Debug, "CVideoDecode = %X 停止解码 line= %d  this= %X Step4 ", this, __LINE__, this);
   }
   if (frameSWS)
   {
	   frameSWS->DeleteAVFrameSws();
	   SAFE_DELETE(frameSWS);
   }
}

//解码
int CVideoDecode::DecodeYV12Image(unsigned char *szImage, int nImageLen)
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
		if (pDPicture->width > 0 && pDPicture->height > 0 && pDPicture->data[0] != NULL && pDPicture->data[1] != NULL && pDPicture->data[2] != NULL)
			return nGet;
		else
			return -1;
	}else 
       return nGet;
}

//支持不显示抓拍
bool CVideoDecode::CaptureJpegFromAVFrame(char* OutputFileName, int quality)
{
	std::lock_guard<std::mutex> lock(m_decodeLockMutex);

	if (m_bInitDecode == false || m_nWidth == 0 || m_nHeight == 0 || pDPicture->data[0] == NULL || pDPicture->data[1] == NULL || pDPicture->data[2] == NULL)
		return false;

	AVFormatContext *pFormatCtx;
	AVStream *video_st;
	AVCodecContext* pCodecCtx;

#ifdef FFMPEG6
	const  AVCodec* pCodec = nullptr;
#else
	AVCodec* pCodec = nullptr;
#endif // FFMPEG6
	AVPacket *pkt;          //编码后数据，如jpeg等
	int     ret1;
  
	if (pDPicture->data[0] == NULL || pDPicture->width <= 0 || pDPicture->height <= 0 || pDPicture->width > 4096 || pDPicture->height > 3000)
	{
		WriteLog(Log_Debug, "CVideoDecode = %X line= %d,", this, __LINE__);
		return false;
	}

	pFormatCtx = NULL ;
	avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, OutputFileName);
	if (pFormatCtx == NULL)
	{
		WriteLog(Log_Debug, "CVideoDecode = %X avformat_alloc_output_context2() Failed line= %d,", this, __LINE__);
		return false;
	}

	if ((ABL_MediaServerPort.snapOutPictureWidth == 0 && ABL_MediaServerPort.snapOutPictureHeight == 0) || ABL_MediaServerPort.snapOutPictureWidth == m_nWidth && ABL_MediaServerPort.snapOutPictureHeight == m_nHeight)
	{
	  m_outWidth  = m_nWidth;
	  m_outHeight = m_nHeight;
	}
	else
	{
		m_outWidth = ABL_MediaServerPort.snapOutPictureWidth;
		m_outHeight = ABL_MediaServerPort.snapOutPictureHeight;
		if (frameSWS == NULL)
		{
			frameSWS = new CAVFrameSWS;
			frameSWS->CreateAVFrameSws(false ,AV_PIX_FMT_YUV420P, m_nWidth, m_nHeight, AV_PIX_FMT_YUV420P, m_outWidth, m_outHeight, SWS_BICUBIC);
		}
	}

	// 获取编解码上下文信息
	pCodecCtx = avcodec_alloc_context3(NULL);

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = m_outWidth;
	pCodecCtx->height = m_outHeight;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	//pCodecCtx->bit_rate = 1024 * 1024 * 3;

	pCodecCtx->bit_rate = m_outWidth * m_outHeight * 3;
	pCodecCtx->qcompress = 1 ; //图片压缩质量 范围（ 0.1 ~ 1 ）
	pCodecCtx->gop_size = 25 ;
	pCodecCtx->max_b_frames = 0;


	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		avcodec_free_context(&pCodecCtx);
		avformat_free_context(pFormatCtx);		
		WriteLog(Log_Debug, "CVideoDecode = %X avcodec_find_encoder() Failed line= %d,", this, __LINE__);
		return false;
	}

	if ((ret1 = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0)
	{
		avcodec_free_context(&pCodecCtx);
		avformat_close_input(&pFormatCtx);		
		avformat_free_context(pFormatCtx);		
		WriteLog(Log_Debug, "CVideoDecode = %X avcodec_open2() Failed line= %d,", this, __LINE__);
		return false;
	}

	// start encoder
	int ret;
	if ((ABL_MediaServerPort.snapOutPictureWidth == 0 && ABL_MediaServerPort.snapOutPictureHeight == 0) || (ABL_MediaServerPort.snapOutPictureWidth == m_nWidth && ABL_MediaServerPort.snapOutPictureHeight == m_nHeight))
	  ret = avcodec_send_frame(pCodecCtx, pDPicture);
	else
	{
		if (frameSWS)
		{
			if (frameSWS->AVFrameSWS(pDPicture))
			{
 			   ret = avcodec_send_frame(pCodecCtx, frameSWS->pFrameDest);
			}
 		}
	}
	if (ret != 0)
	{
		avcodec_close(pCodecCtx);
		avcodec_free_context(&pCodecCtx);
		avformat_free_context(pFormatCtx);

		WriteLog(Log_Debug, "CVideoDecode = %X avcodec_send_frame() Failed line= %d,", this, __LINE__);
		return false;
	}

	pkt = av_packet_alloc();
	av_new_packet(pkt, m_outWidth*m_outHeight * 3);

	//Read encoded data from the encoder.
	ret = avcodec_receive_packet(pCodecCtx, pkt);
	if (ret != 0)
	{
		avcodec_close(pCodecCtx);
		avcodec_free_context(&pCodecCtx);
		avformat_close_input(&pFormatCtx);
		avformat_free_context(pFormatCtx);
		av_packet_free(&pkt);

		WriteLog(Log_Debug, "CVideoDecode = %X avcodec_receive_packet() Failed line= %d,", this, __LINE__);
		return false;
	}

	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st == NULL)
	{
		avcodec_close(pCodecCtx);
		avcodec_free_context(&pCodecCtx);
		avformat_close_input(&pFormatCtx);
		avformat_free_context(pFormatCtx);
		av_packet_free(&pkt);

		WriteLog(Log_Debug, "CVideoDecode = %X avformat_new_stream() Failed  .", this);
		return false;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	//Write body
	av_write_frame(pFormatCtx, pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);

	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);

	av_packet_free(&pkt);
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
  
	return true;
}
