#if !defined(AFX_VIDEODECODE_H__D579E9D1_0B06_4891_99FB_BFCDC279261C__INCLUDED_)
#define AFX_VIDEODECODE_H__D579E9D1_0B06_4891_99FB_BFCDC279261C__INCLUDED_

#include "AVFrameSWS.h"

class CVideoDecode  
{
public:
	CVideoDecode();
	virtual           ~CVideoDecode();
	int                DecodeYV12Image(unsigned char *szImage, int nImageLen);
	void               stopDecode();
	bool               startDecode(char* szCodecName, int nWidth, int nHeight);
	bool               CaptureJpegFromAVFrame(char* OutputFileName, int quality);

	int                m_outWidth, m_outHeight;
	CAVFrameSWS*       frameSWS;
	uint64_t           nPort;//解码通道
	std::mutex         m_decodeLockMutex;

	int                m_nWidth, m_nHeight;
	char               m_szCodecName[64];
	int                nFrameGopCount ;

	//标准解码器 h264,mpeg4,MJPEG,h265
	int                nDecodeErrorCount; //解码出错次数
	AVPacket*          packet;
	int                nGet,frameFinished ;
	bool               m_bInitDecode ;


#ifdef FFMPEG6
	const  AVCodec* pDCodec = nullptr;
#else
	AVCodec* pDCodec = nullptr;
#endif // FFMPEG6

	AVCodecContext*    pDCodecCtx ;
	AVFrame*           pDPicture ;
	int                nRet;
};

#endif // !defined(AFX_VIDEODECODE_H__D579E9D1_0B06_4891_99FB_BFCDC279261C__INCLUDED_)
