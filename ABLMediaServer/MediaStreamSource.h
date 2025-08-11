#ifndef _MediaStreamSource_H
#define _MediaStreamSource_H

#define   MediaStreamSource_VideoFifoLength   1024*1024*3  //��Ƶ�洢����
#define   MediaStreamSource_AudioFifoLength   1024*256     //��Ƶ�洢���� 
#define   Default_TS_MediaFileByteCount       1024*1024*3  //ȱʡ����Ƭ�ļ���С 
#define   MaxStoreTsFileCount                 4            //���������TS�ļ�������

#include "NetRecvBase.h"
#include "hls-fmp4.h"
#include "mpeg-ps.h"
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mov-format.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"

#include "mov-format.h"
#include "fmp4-writer.h"

#include "MediaFifo.h"
#include "mpeg4-hevc.h"
#include "mpeg4-aac.h"
#include "g711_table.h"
#include "AACEncode.h"
#include "FFVideoDecode.h"
#include "FFVideoEncode.h"
#include <unordered_map>
//#define  WriteCudaDecodeYUVFlag    1
//#define    WriteInputVdideoFlag      1
//#define   WriteInputVideoFileFlag      1 

#define   CudaDecodeH264EncodeH264FIFOBufferLength  2048*1024*1

class CNetRevcBase;

//���ڼ�¼rtsp����ʱ��SDP��Ϣ
struct RtspSDPContentStruct
{
	char szSDPContent[string_length_1024];
	char szVideoName[64];
	int  nVidePayload;

	char szAudioName[64];
	int  nAudioPayload;
	int  nChannels;
	int  nSampleRate;
	RtspSDPContentStruct()
	{
		memset(szSDPContent, 0x00, sizeof(szSDPContent));
		memset(szVideoName, 0x00, sizeof(szVideoName));
		memset(szAudioName, 0x00, sizeof(szAudioName));

		nVidePayload = 0;
		nAudioPayload = 0;
		nChannels = 0;
		nSampleRate = 0 ;
	}
};

#define  OneFrame_MP4_Stream_BufferLength    1024*1024*2 

class CMediaStreamSource
{
public:
   CMediaStreamSource(char* szURL,uint64_t nClientTemp, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct h265ConvertH264Struct);
   ~CMediaStreamSource();

   bool            initiative;//�Ƿ������Ͽ�  
   static int32_t  mediaSourceCount;//������������
   void            SetG711ConvertAAC(int nFlag);
   int             nG711ConvertAAC;//�Ƿ�ת�룬Ĭ�ϴ������ļ���ȡ��Ҳ�����ⲿ�������� 
   int             videoFileFormat;//¼��洢��ʽ��׼��ÿһ��������Ƶ 
   char            sourceURL[string_length_2048];//Դ���ṩ�ߵ�URL 
   void            addClientToDisconnectFifo();
#ifdef WriteInputVideoFileFlag
   FILE*   fWriteInputVideoFile;
#endif
   volatile bool   bEnableFlag ;
   bool            CopyAudioFrameBufer();
   bool            CopyVideoGopFrameBufer();//����һ��gop��Ƶ֡ 

   CMediaFifo      pCacheAudioFifo;
   CMediaFifo      pCopyCacheAudioFifo;
   char            szSnapPicturePath[string_length_512];
   uint64_t        iFrameArriveNoticCount;
   uint64_t        nCreateDateTime;
   volatile bool   m_bNoticeOnPublish;//�����¼��Ƿ�֪ͨ
   volatile bool   m_bPauseFlag;//ý��Դ�Ƿ���ͣ����

   bool  SetPause(bool bFlag);
   bool  FFMPEGGetWidthHeight(unsigned char * videooutdata, int videooutdatasize, char* videoName, int * outwidth, int * outheight);

#ifdef  WriteInputVdideoFlag
   FILE*   fWriteInputVideo;
#endif
#ifdef WriteCudaDecodeYUVFlag
   int                    nWriteYUVCount;
   FILE*                  fCudaWriteYUVFile;
#endif
   int                    nSrcWidth, nSrcHeight;//����ԭʼ��Ƶ���� 

   H265ConvertH264Struct  m_h265ConvertH264Struct;
   bool                   ChangeVideoFilter(char *filterText, int fontSize, char *fontColor, float fontAlpha, int fontLeft, int fontTop);
   CFFVideoFilter*        pFFVideoFilter ;        
   uint64_t               nEncodeCudaChan ;

   void                   GetCreateTSDateTime();
   char                   szCreateTSDateTime[128];//����ts�ļ�ʱ��
   volatile  bool         enable_hls;
   void                   InitHlsResoure();
   volatile bool          bInitHlsResoureFlag;
   volatile bool          bNoticeClientArriveFlag;//�Ƿ�֪ͨ��������
   unsigned int           nVideoBitrate;//��Ƶ����
   unsigned int           nAudioBitrate;//��Ƶ����
   volatile uint64_t      nCalcBitrateTimestamp;//��������ʱ���
   CMediaFifo             pVideoGopFrameBuffer;//���������һ��Gop������Ƶ֡
   CMediaFifo             pCopyVideoGopFrameBuffer;//�������������һ��Gop������Ƶ֡
   int                    nIDRFrameLengh;//���µ�һ��I֡���� 
   int                    nCudaWidth, nCudaHeight;
   uint64_t               nCudaDecodeChan;//cuda����ͨ��
   int                    nCudaDecodeFrameCount;//����ɹ����֡����
   int                    nEncodeBufferLengthCount;//ת���buffer���ܳ���
#ifdef OS_System_Windows
   unsigned char*        pCudaDecodeYUVFrame;//����ɹ���֡����
#else
   unsigned char*         pCudaDecodeYUVFrame;//����ɹ���֡����	
#endif
   int                    nCudeDecodeOutLength;//����ɹ���ĳ���

   unsigned char*         pOutEncodeBuffer;
   int                    nOneFrameLength;
   int                    nGetFrameCountLength;
   CFFVideoDecode         videoDecode;
   CAVFrameSWS            avFrameSWS;
   CFFVideoEncode         videoEncode;
   int                    nOutLength;
   volatile  bool         H265ConvertH264_enable; //�Ƿ�������ת��
   static int             nConvertObjectCount;//����ת��·�� 
   bool                   H265ConvertH264(unsigned char* szVideo, int nLength, char* szVideoCodec);

   uint64_t               tCopyVideoTime; //������Ƶʱ��� 
   int                    netBaseNetType; //ý���ṩ�ߵĶ�����������
   char                   szJson[string_length_4096] ;  //���ɵ�json

   bool                   ConvertG711ToAAC(int nCodec, unsigned char* pG711, int nBytes,unsigned char* szOutAAC, int& nAACLength);
   CAACEncode             aacEnc;
   unsigned char          pOutAACData[2048];
   int                    nOutAACDataLength;
   char                   g711ToPCMCache[1024 * 16];
   unsigned char          g711CacheBuffer[8192];
   int                    nG711CacheLength;
   int                    nG711CacheProcessLength;//�и�ǰ�ܳ��� 
   int                    nG711SplittePos; //�и��ƶ�λ�� 
   int                    nG711ToPCMCacheLength;
   int                    nAACEncodeLength;
   int                    nRetunEncodeLength;
   char                   g711toPCM[string_length_2048];
   char                   g711toPCMResample[2048];

   MediaSourceType        nMediaSourceType;//ý��Դ���ͣ�ʵ�����ţ�¼��㲥
   uint32_t               nMediaDuration;//ý��Դʱ������λ�룬��¼��㲥ʱ��Ч
   uint64_t               fileKeepMaxTime;//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 

   char                   szRecordPath[string_length_2048];
   volatile   bool        enable_mp4;//�Ƿ�¼��mp4�ļ�
   uint64_t               recordMP4;

   bool                   GetVideoWidthHeight(char* szVideoCodeName,unsigned char* pVideoData, int nDataLength);

   void                   UpdateVideoFrameSpeed(int nVideoSpeed,int netType);
	   
   std::mutex             mediaTsMp4CutLock; //ts FMP ��Ƭ�� 
   int64_t                nTsCutFileCount;
   volatile      bool     hls_init_segmentFlag;
   void                   SaveTsMp4M3u8File(); //ͳһ����TS��MP4��M3u8 �ļ� 
   unsigned char          pFmp4SPSPPSBuffer[1024 * 128];
   int                    nFmp4SPSPPSLength;

   struct mpeg4_hevc_t    hevc; 
   bool                   H265FrameToFMP4File(unsigned char* szVideoData, int nLength); 
   unsigned  int          FindSpsPosition(char* szVideoCodeName, unsigned char* szVideoBuffer, int nBufferLength, bool &bFind);

   unsigned   char*       pTsFileCacheBuffer;//TS��Ƭ��ʱ������
   int                    nMaxTsFileCacheBufferSize; //��ǰpTsFileCacheBuffer �ֽڴ�С 

   char                  s_bufferH264TS[188];
   int64_t               nVideoOrder ;
   int64_t               nAudioOrder ;
   struct mpeg_ts_func_t tshandler;
   CMediaFifo            tsFileNameFifo;
   int64_t               tsCreateTime;
   CMediaFifo            m3u8FileFifo;
   int64_t               m3u8FileOrder;
   void*                 tsPacketHandle ;
   FILE*                 fTSFileWrite;
   int                   fTSFileWriteByteCount;
   char                  szOutputName[string_length_2048];
   char                  szHookTSFileName[string_length_2048];
   char                  szH264TempBuffer[string_length_2048];
   char                  szM3u8Buffer[string_length_2048];
   int                   avtype;
   int                   flags;
   int64_t               ptsVideo;
   std::map<int, int>    streamsTS;
   int                   ts_stream(void* ts, int codecid);
   bool                  H264H265FrameToTSFile(unsigned char* szVideo, int nLength);
 
   int                   nVideoStampAdd;//��Ƶʱ�������
   int64_t               nAsyncAudioStamp;//ͬ����ʱ���

   bool                  GetRtspSDPContent(RtspSDPContentStruct* sdpContent);
   RtspSDPContentStruct  rtspSDPContent;
   unsigned char         pSPSPPSBuffer[string_length_2048];
   int                   nSPSPPSLength; 

   //����Ƿ���I֡
   bool                bIFrameFlag;
   bool                CheckVideoIsIFrame(char* szVideoCodecName ,unsigned char* szPVideoData, int nPVideoLength);
   unsigned char       szVideoFrameHead[4];

   bool                 CopyTsFileBuffer(int64_t nTsFileNameOrder, unsigned char* pOutTsBuffer);
   CMediaFifo           mediaFileBuffer[MaxStoreTsFileCount];//��Ƭ���ڴ�ʱ�Ĵ洢FIFO
   int                  nTsFileSizeArray[MaxStoreTsFileCount];//ts��Ƭ�ļ���Ӧ���ļ��ֽڴ�С
   int                  GetTsFileSizeByOrder(int64_t nTsFileNameOrder);

   std::mutex           mediaTsFileLock;//ý����Ƭ��
   char                 szTempBuffer[string_length_2048];
   void                 CopyM3u8Buffer(char* szM3u8Buffer);
   bool                 ReturnM3u8Buffer(char* szOutM3u8);

   void                 ABLDeletePath(char* szDeletePath,char* srcPath);
   void                 CreateSubPathByURL(char* szMediaURL);
   char                 szTSFileSubPath[string_length_2048];//TS�ļ�����·�������ڲ���TS�ļ���·��
   char                 szHLSPath[string_length_2048]; //HLS·����������·�������� D:\ABLMediaServer\www\Media\Camera_00001\

   unsigned char*       pH265Buffer;
   int                  nMp4BufferLength;
   int                  vcl  ;
   int                  update  ;
   unsigned char        s_packet[1024*16];
   unsigned char        szExtenVideoData[4*1024];
   int                  extra_data_sizeH265;
   hls_fmp4_t*          hlsFMP4; 
   int                  track_265 ;
   struct mpeg4_aac_t   aacHandle;
   int                  track_aac ;
   unsigned char        szExtenAudioData[string_length_2048];
   int                  nExtenAudioDataLength;
   int                  nAACLength;

   int64_t              s_dts ;
   int                  nTsFileCount;
   int64_t              nTsFileOrder;
   int64_t              videoDts;
   int64_t              audioDts;
   char                 szDataM3U8[48 * 1024];
   char                 szTsName[string_length_2048] ;
   char                 szM3u8Name[string_length_2048];
   char                 szTempName[string_length_2048];

   bool                 PushVideo(unsigned char* szVideo, int nLength, char* szVideoCodec);
   bool                 PushAudio(unsigned char* szAudio, int nLength, char* szAudioCodec, int nChannels, int SampleRate);
   
   bool                 AddClientToMap(NETHANDLE nClient);
   bool                 DeleteClientFromMap(NETHANDLE nClient);

   char                 m_szURL[string_length_2048]; //����  /Media/Camera_00001     /Live/Camera_00001 ,  url ����������2�� 
   char                 app[string_length_1024];
   char                 stream[string_length_1024];
   MediaCodecInfo       m_mediaCodecInfo;
   char                 sim[string_length_256];

   std::mutex           mediaSendMapLock;

#ifdef USE_BOOST
   boost::unordered_map<NETHANDLE, boost::shared_ptr<CNetRevcBase> > mediaSendMap;//������ ��Ҫ ���͡������������б�  

#else
   std::unordered_map<NETHANDLE, std::shared_ptr<CNetRevcBase> > mediaSendMap;//������ ��Ҫ ���͡������������б�  

#endif

   uint64_t             nClient; //��¼���Ǹ����ӽ��յ����� 
   uint64_t             nLastWatchTime, nRecordLastWatchTime, nLastWatchTimeDisconect;//���ۿ�ʱ��
   volatile bool        bUpdateVideoSpeed;//�Ƿ������Ƶ�ٶ�
};

#endif
