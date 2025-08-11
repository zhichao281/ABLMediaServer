#ifndef _StreamRecordFMP4_H
#define _StreamRecordFMP4_H

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
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif
class CStreamRecordFMP4 : public CNetRevcBase
{
public:
	CStreamRecordFMP4(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CStreamRecordFMP4() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pAudioData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����
   virtual int SendVideo();//������Ƶ����
   virtual int SendAudio();//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   unsigned char*          pWriteDiskRecordBuffer;//д��Ӳ��¼�񻺴� 
   char                    szFileNameOrder[64];
   bool                    writeTSBufferToMP4File(unsigned char* pTSData, int nLength);
   char                    szFileName[512] ;

   volatile    bool        bWaitIFrameSuccessFlag;
   volatile    bool        bAddSendThreadToolFlag;
   char                    httpResponseData[string_length_4096];
   unsigned char           netDataCache[1024*32]; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;
   int                     data_Length;
   char                    szMP4Name[512];
   volatile bool           bFindMP4NameFlag;
   volatile  bool          bCheckHttpMP4Flag; //����Ƿ�Ϊhttp-MP4Э�� 
   int                     nWriteRet;

   FILE*                 fWriteMP4;
   std::mutex            mediaMP4MapLock;
   int                   avtype;

   unsigned char        pH265Buffer[MediaStreamSource_VideoFifoLength];
   int                  nMp4BufferLength;
   int                  vcl;
   int                  update;
   unsigned char        s_packet[1024*256];
   int                  s_packetLength;
   unsigned char        szExtenVideoData[4 * 1024];
   int                  extra_data_size;
   hls_fmp4_t*          hlsFMP4;
   int                  track_video;
   struct mpeg4_aac_t   aacHandle;
   int                  track_aac;
   unsigned char        szExtenAudioData[256];
   int                  nExtenAudioDataLength;
   int                  nAACLength;
   int                   flags;
   volatile      bool     hls_init_segmentFlag;
   int                    nFmp4SPSPPSLength;
   int                    fTSFileWriteByteCount;

   struct mpeg4_hevc_t    hevc;
   struct mpeg4_avc_t     avc;
   bool                   VideoFrameToFMP4File(unsigned char* szVideoData, int nLength);
};

#endif