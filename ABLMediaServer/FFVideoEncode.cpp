/*
功能：  实现ffmpeg 硬编码，或者软编码 

Author 罗家兄弟
Date   2022-02-19
QQ     79941308
E-mail 79941308@qq.com

*/
#include "stdafx.h"
#include "FFVideoEncode.h"

extern MediaServerPort                       ABL_MediaServerPort;
#ifdef OS_System_Windows
extern BOOL GBK2UTF8(char *szGbk, char *szUtf8, int Len);
#else
extern int GB2312ToUTF8(char* szSrc, size_t iSrcLen, char* szDst, size_t iDstLen);
#endif

CFFVideoEncode::CFFVideoEncode()
{
	m_nAVPixel = AV_PIX_FMT_NV12;
	m_bInitFlag = enableFilter = false;
	videoFilter = NULL;
	WriteLog(Log_Debug, "CFFVideoEncode 构造函数 = %X 创建编码器成功 line= %d ", this, __LINE__);
}

CFFVideoEncode::~CFFVideoEncode()
{
	StopEncode();
	WriteLog(Log_Debug, "CFFVideoEncode 析构函数 = %X 释放编码器成功 line= %d ", this, __LINE__);
	malloc_trim(0);
}
/*
功能：
初始化类
参数：
char*          szEncodeName  编码器名称 比如libx264 , h264_qsv ,
AVPixelFormat  nAVPixel      YUV输入格式    AV_PIX_FMT_YUV420P 、 AV_PIX_FMT_NV12
比如 libx264  对应 需要 AV_PIX_FMT_YUV420P ，h264_qsv 对应需要 AV_PIX_FMT_NV12
int            nWidth        YUV 宽
int            nHeight       YUV 高
int           nFrameRate     视频帧速度
*/
bool CFFVideoEncode::StartEncode(char* szEncodeName, AVPixelFormat nAVPixel, int nWidth, int nHeight, int nFrameRate, int nEncodeByteRate)
{
	std::lock_guard<std::mutex> lock(enable_Lock);
	 WriteLog(Log_Debug, "StartEncode nAVPixel = %d ", (int)nAVPixel);
	 strcpy(m_szEncodeName, szEncodeName);
	 m_nAVPixel = nAVPixel;
	 m_nWidth = nWidth ;
	 m_nHeight = nHeight ;
	 m_nFrameRate = nFrameRate;

	 AVDictionary* param = 0;

	 //查找编码器
	 pCodec = avcodec_find_encoder_by_name(szEncodeName);

	 if (!pCodec) 
	 {
		 printf("Can not find encoder! \n");
		 return false;
	 }
	
	 pCodecCtx = avcodec_alloc_context3(pCodec);
	 if (!pCodecCtx) {
		 printf("Can not allocate video codec context! \n");
		 return false ;
	 }

	 pCodecCtx->codec_id = AV_CODEC_ID_H264;
	 pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	 pCodecCtx->pix_fmt = nAVPixel;// AV_PIX_FMT_NV12  AV_PIX_FMT_YUV420P;
	 pCodecCtx->width = m_nWidth;
	 pCodecCtx->height = m_nHeight;
	 pCodecCtx->bit_rate = nEncodeByteRate * 1024 ;
	 pCodecCtx->gop_size = 25;

	 pCodecCtx->time_base = { 1, m_nFrameRate };
	// pCodecCtx->time_base.num = 1;
	 //pCodecCtx->time_base.den = m_nFrameRate;

	 pCodecCtx->qmin = 10;
	 pCodecCtx->qmax = 51;
	 pCodecCtx->slice_count = 4;
	 pCodecCtx->thread_count = 4;

	 pCodecCtx->max_b_frames = 0;

	 //H.264
	 if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		 av_dict_set(&param, "preset", "ultrafast", 0);
		 av_dict_set(&param, "tune", "zerolatency", 0);
	 }
	 if (pCodecCtx->codec_id == AV_CODEC_ID_HEVC) {
		 av_dict_set(&param, "preset", "ultrafast", 0);
		 av_dict_set(&param, "tune", "zerolatency", 0);
	 }

	 //打开编码器
	 if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) 
	 {
		 printf("Failed to open encoder! \n");
		 avcodec_free_context(&pCodecCtx);
		 return false ;
	 }

	 pFrame = av_frame_alloc();
	 picture_size = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,1);
	 picture_buf = (uint8_t*)av_malloc(picture_size);
	 av_image_fill_arrays(pFrame->data, pFrame->linesize, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);

	 av_new_packet(&pkt, picture_size);
 
	 pFrame->format = pCodecCtx->pix_fmt;
	 pFrame->width = pCodecCtx->width;
	 pFrame->height = pCodecCtx->height;

	 //水印功能设置
	 char szFilterUTF8[1280] = { 0 };
#ifdef OS_System_Windows 
	 GBK2UTF8(ABL_MediaServerPort.filterVideoText, szFilterUTF8, sizeof(szFilterUTF8));
#else
	 GB2312ToUTF8(ABL_MediaServerPort.filterVideoText,strlen(ABL_MediaServerPort.filterVideoText), szFilterUTF8, sizeof(szFilterUTF8));
#endif
	 enableFilter = false;
	 std::string filterText(szFilterUTF8);
	 if (ABL_MediaServerPort.filterVideo_enable == 1 && !filterText.empty())
	 {
		 videoFilter = new CFFVideoFilter;
		 videoFilter->waterMarkText = filterText;
		 videoFilter->StartFilter(nAVPixel, nWidth, nHeight, nFrameRate, ABL_MediaServerPort.nFilterFontSize, ABL_MediaServerPort.nFilterFontColor, ABL_MediaServerPort.nFilterFontAlpha, ABL_MediaServerPort.nFilterFontLeft, ABL_MediaServerPort.nFilterFontTop);
		 enableFilter = true;
	 }

	 m_bInitFlag = true;
	 return true;
}

bool  CFFVideoEncode::EncodecYUV(unsigned char* pYUVData, int nLength, unsigned char* pEncodecData, int* nOutLength)
{
	std::lock_guard<std::mutex> lock(enable_Lock);
	if (m_bInitFlag)
	{
		got_picture = 0;
		pFrame->pts = nFrameNumber * (pCodecCtx->time_base.den) / ((pCodecCtx->time_base.num) * 25);
		memcpy(picture_buf, pYUVData, nLength);
		nFrameNumber ++;

		//执行水印操作
		AVFrame *srcFrame = pFrame;
		if (enableFilter)
		{
			if (videoFilter->FilteringFrame(pFrame))
				srcFrame = videoFilter->filterFrame;
		}

 		ret = avcodec_send_frame(pCodecCtx, srcFrame);
		if (ret < 0) {
			av_packet_unref(&pkt);
			return false;
 		}

		while (ret >= 0) {
			ret = avcodec_receive_packet(pCodecCtx, &pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
			{
				av_packet_unref(&pkt);
				return false ;
			}
 
			memcpy(pEncodecData, pkt.data, pkt.size);
			*nOutLength = pkt.size;

 			av_packet_unref(&pkt);
			return true;
		}

		return false ;
	}
	else
	{
		return false;
	}
}

bool  CFFVideoEncode::EncodecAVFrame(AVFrame* inAVFrame, unsigned char* pEncodecData, int* nOutLength)
{
	std::lock_guard<std::mutex> lock(enable_Lock);
	if (m_bInitFlag)
	{
		//执行水印操作
		AVFrame *srcFrame = inAVFrame;
		if (enableFilter)
		{
			if (videoFilter->FilteringFrame(inAVFrame))
				srcFrame = videoFilter->filterFrame;
		}

		got_picture = 0;
		srcFrame->pts = nFrameNumber * (pCodecCtx->time_base.den) / ((pCodecCtx->time_base.num) * 25);
 
		ret = avcodec_send_frame(pCodecCtx, srcFrame);
		if (ret < 0) {
			av_packet_unref(&pkt);
			return false;
		}

		while (ret >= 0) {
			ret = avcodec_receive_packet(pCodecCtx, &pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
			{
				av_packet_unref(&pkt);
				return false;
			}

			memcpy(pEncodecData, pkt.data, pkt.size);
			*nOutLength = pkt.size;
			av_packet_unref(&pkt);
			return true;
		}

		nFrameNumber ++;

		return true;
	}
	else
	{
		return false;
	}
}

bool CFFVideoEncode::ChangeVideoFilter(char *filterText, int fontSize, char *fontColor, float fontAlpha, int fontLeft, int fontTop)
{
	std::lock_guard<std::mutex> lock(enable_Lock);

	enableFilter = false;
	std::string tmpText(filterText);

	//此时不考虑全局水印是否启用
	if (tmpText.empty())
		return false;

	if (videoFilter)
		SAFE_DELETE(videoFilter);

	videoFilter = new CFFVideoFilter;
	videoFilter->waterMarkText = tmpText;
	videoFilter->nFilterFontSize = fontSize;
	strncpy(videoFilter->nFilterFontColor, fontColor, 64);
	videoFilter->nFilterFontAlpha = fontAlpha;
	videoFilter->nFilterFontLeft = fontLeft;
	videoFilter->nFilterFontTop = fontTop;

	videoFilter->StartFilter(m_nAVPixel, m_nWidth, m_nHeight, m_nFrameRate, videoFilter->nFilterFontSize, videoFilter->nFilterFontColor, videoFilter->nFilterFontAlpha, videoFilter->nFilterFontLeft, videoFilter->nFilterFontTop);
	enableFilter = true;
	WriteLog(Log_Debug, "HttpRequest ChangeVideoFilter");

	return true;
}
bool  CFFVideoEncode::StopEncode()
{
	std::lock_guard<std::mutex> lock(enable_Lock);
	if (m_bInitFlag)
	 {
 	   av_packet_unref(&pkt);
	   av_frame_free(&pFrame);
	   av_free(picture_buf);
	   avcodec_free_context(&pCodecCtx);

	   SAFE_DELETE(videoFilter);
	   m_bInitFlag = false ;
	 }

	return true;
}


//Add by ZXT
CFFVideoFilter::CFFVideoFilter()
{
	bRunFlag = false ;
	waterMarkText = "ABL";
#ifdef OS_System_Windows 
	 
#else
	 char szFilterUTF8[256]={0};
	 GB2312ToUTF8(ABL_MediaServerPort.filterVideoText,strlen(ABL_MediaServerPort.filterVideoText), szFilterUTF8, sizeof(szFilterUTF8));
	 waterMarkText = szFilterUTF8 ;
#endif

	filterOutputs = NULL;
	filterInputs = NULL;
	filter_graph = NULL;
	filterFrame = NULL;
	nFilterFontSize = ABL_MediaServerPort.nFilterFontSize;
	strcpy(nFilterFontColor, ABL_MediaServerPort.nFilterFontColor);
	nFilterFontAlpha = ABL_MediaServerPort.nFilterFontAlpha;;//透明度
	nFilterFontLeft = ABL_MediaServerPort.nFilterFontLeft;;//x坐标
	nFilterFontTop = ABL_MediaServerPort.nFilterFontTop;;//y坐标
}

CFFVideoFilter::~CFFVideoFilter()
{
	StopFilter();
	malloc_trim(0);
}

bool CFFVideoFilter::StartFilter(AVPixelFormat nAVPixel, int nWidth, int nHeight, int nFrameRate,int nFontSize,char* FontColor,float FontAlpha,int FontLeft,int FontTop)
{
	if(bRunFlag)
		return true ;
	
	m_nWidth  = nWidth ;
	m_nHeight = nHeight ;
	
	//文本水印
	char szFilter[2048] = { 0 };
	sprintf(szFilter, "drawtext=fontfile=./simhei.ttf:fontsize=%d:fontcolor=%s:alpha=%.1f:x=%d:y=%d:text=", nFontSize,  FontColor, FontAlpha, FontLeft, FontTop);
	WriteLog(Log_Debug, "StartFilter  szFilter = %s  ", szFilter);

	std::string filters_descr = szFilter ;
	filters_descr += waterMarkText;

	//图片水印
	//std::string filters_descr = "movie=./watermark/logo.png[watermark];[in][watermark]overlay=10:10[out]";

	enum AVPixelFormat pix_fmts[] = { nAVPixel, AV_PIX_FMT_NONE };
	const AVFilter *buffersrc = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");

	filterOutputs = avfilter_inout_alloc();
	filterInputs = avfilter_inout_alloc();
	filter_graph = avfilter_graph_alloc();
	if (!filterOutputs || !filterInputs || !filter_graph)
	{
		fprintf(stderr, "avfilter inout/graph alloc error\n");
		return false;
	}

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	char args[512];
	sprintf(args, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		nWidth, nHeight, nAVPixel, 1, nFrameRate, 1, 1);

	int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
	if (ret < 0)
	{
		fprintf(stderr, "avfilter_graph_create_filter error\n");
		return false;
	}

	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
	if (ret < 0)
	{
		fprintf(stderr, "avfilter_graph_create_filter error\n");
		return false;
	}

	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		fprintf(stderr, "av_opt_set_int_list error\n");
		return false;
	}

	/* Endpoints for the filter graph. */
	filterOutputs->name = av_strdup("in");
	filterOutputs->filter_ctx = buffersrc_ctx;
	filterOutputs->pad_idx = 0;
	filterOutputs->next = NULL;

	filterInputs->name = av_strdup("out");
	filterInputs->filter_ctx = buffersink_ctx;
	filterInputs->pad_idx = 0;
	filterInputs->next = NULL;

	ret = avfilter_graph_parse_ptr(filter_graph, filters_descr.c_str(), &filterInputs, &filterOutputs, NULL);
	if (ret < 0)
	{
		fprintf(stderr, "avfilter_graph_parse_ptr error");
		return false;
	}

	ret = avfilter_graph_config(filter_graph, NULL);
	if (ret < 0)
	{
		fprintf(stderr, "avfilter_graph_config error\n");
		return false;
	}

	filterFrame = av_frame_alloc();
     
	bRunFlag = true ; 
	return true;
}

bool CFFVideoFilter::StopFilter()
{
	if(bRunFlag)
	{
		if (filterInputs != NULL)
		{
			avfilter_inout_free(&filterInputs);
			filterInputs = NULL;
		}
		if (filterOutputs != NULL)
		{
			avfilter_inout_free(&filterOutputs);
			filterInputs = NULL;
		}
		if (filter_graph != NULL)
		{
			avfilter_graph_free(&filter_graph);
			filter_graph = NULL;
		}
		if (filterFrame != NULL)
		{
			av_frame_free(&filterFrame);
			filterFrame = NULL;
		}
		bRunFlag = false ;
		return true;
	}
}

bool CFFVideoFilter::FilteringFrame(AVFrame * srcFrame)
{
	if(!bRunFlag)
		return false ;
	
	//释放已存在的数据
	av_frame_unref(filterFrame);

	//解码后的frame添加至filtergraph
	if (av_buffersrc_add_frame(buffersrc_ctx, srcFrame) < 0)
	{
		fprintf(stderr, "Error while feeding the filtergraph\n");
		return false;
	}

	//filtergraph中取出处理好的frame
	if (av_buffersink_get_frame(buffersink_ctx, filterFrame) < 0)
	{
		fprintf(stderr, "av_buffersink_get_frame error\n");
		return false;
	}

	return true;
}

bool CFFVideoFilter::CopyYUVData(unsigned char* pOutYUVData)
{
	if(!bRunFlag || filterFrame == NULL )
		return false ;
	
	if(filterFrame->data[0] == NULL || filterFrame->data[1] == NULL || filterFrame->data[2] == NULL)
		return false ;
	
	  nYUVPos = 0;
	  nYPos = 0;
	  nUPos = 0;
	  nVPos = 0;
	  
	  for (i = 0; i<m_nHeight; i++)
	  {
		  memcpy(pOutYUVData + nYUVPos, filterFrame->data[0] + nYPos, m_nWidth);
		  nYPos += filterFrame->linesize[0];
		  nYUVPos += m_nWidth;
	  } 

	  // fill U data
	  for (i = 0; i<m_nHeight / 2; i++)
	  {
		  memcpy(pOutYUVData + nYUVPos, filterFrame->data[1] + nUPos, m_nWidth / 2);
		  nUPos += filterFrame->linesize[1];
		  nYUVPos += m_nWidth / 2;
	  }

	  // fill V data
	  for (i = 0; i<m_nHeight / 2; i++)
	  {
		  memcpy(pOutYUVData + nYUVPos, filterFrame->data[2] + nVPos, m_nWidth / 2);
		  nVPos += filterFrame->linesize[2];
		  nYUVPos += m_nWidth / 2;
	  }	
}


