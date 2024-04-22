#ifndef _ffVideoEncode_H
#define _ffVideoEncode_H
#include <atomic>
class CFFVideoFilter;

class CFFVideoEncode
{
public:
	CFFVideoEncode();
   ~CFFVideoEncode();

   bool             StartEncode(char* szEncodeName, AVPixelFormat nAVPixel, int nWidth, int nHeight,int nFrameRate,int nEncodeByteRate);
   bool             StopEncode();
   bool             EncodecYUV(unsigned char* pYUVData, int nLength,unsigned char* pEncodecData,int* nOutLength);
   bool             EncodecAVFrame(AVFrame* inAVFrame, unsigned char* pEncodecData, int* nOutLength);

   bool				ChangeVideoFilter(char *filterText, int fontSize, char *fontColor, float fontAlpha, int fontLeft, int fontTop); //add
   char					m_szEncodeName[256];
   AVPixelFormat		m_nAVPixel;
   int					m_nWidth;
   int					m_nHeight;
   int					m_nFrameRate;
   std::atomic<bool>    m_bInitFlag;
   int					got_picture ;
   uint64_t				nFrameNumber;
   int					ret;

   AVCodecContext* pCodecCtx;
 

#ifdef FFMPEG6
   const  AVCodec* pCodec = nullptr;
#else
   AVCodec* pCodec = nullptr;
#endif // FFMPEG6

   AVPacket        pkt;
   uint8_t*        picture_buf;
   AVFrame*        pFrame;
   int             picture_size;

   bool            enableFilter;             //启用水印
   CFFVideoFilter  *videoFilter;  //新增水印类

   std::mutex      enable_Lock;
};

//Add by ZXT
class CFFVideoFilter
{
public:
	CFFVideoFilter();
	~CFFVideoFilter();

	//初始化Filter
	bool StartFilter(AVPixelFormat nAVPixel, int nWidth, int nHeight, int nFrameRate, int nFontSize, char* FontColor, float FontAlpha, int FontLeft, int FontTop);
	//释放Filter
	bool StopFilter();

	//执行水印操作
	bool FilteringFrame(AVFrame *srcFrame);
	
	bool CopyYUVData(unsigned char* pOutYUVData);

	std::string waterMarkText;        //水印内容
	AVFilterGraph *filter_graph;      //过滤器graph
	AVFilterContext *buffersink_ctx;  //过滤器上下文
	AVFilterContext *buffersrc_ctx;
	AVFilterInOut *filterOutputs;     //过滤器IO设置
	AVFilterInOut *filterInputs;
	AVFrame *filterFrame;			  //水印帧

	int  nFilterFontSize;
	char  nFilterFontColor[64];
	float nFilterFontAlpha;//透明度
	int  nFilterFontLeft;//x坐标
	int  nFilterFontTop;//y坐标
	
	volatile bool  bRunFlag ;
	int nYUVPos  ;
	int nYPos  ;
	int nUPos  ;
	int nVPos ;	
	int i ;
	int m_nWidth,m_nHeight;
};

#endif