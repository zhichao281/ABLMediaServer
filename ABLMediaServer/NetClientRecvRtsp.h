#ifndef _NetClientRecvRtsp_H
#define _NetClientRecvRtsp_H

#include "DigestAuthentication.hh"

#include "MediaStreamSource.h"
#include "rtp_packet.h"
#include "RtcpPacket.h"
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#else

#endif


#include "mpeg4-avc.h"
#include "rtp-payload.h"
#include "rtp-profile.h"

#define  MaxRtspValueCount          64 
#define  MaxRtspProtectCount        24 
#define   MaxRtpHandleCount         2

//url���ͣ���ʵ��������¼�� ,��������
enum XHRtspURLType
{
	XHRtspURLType_Liveing = 0,  //ʵ������
	XHRtspURLType_RecordPlay = 1,   //¼�񲥷�
	XHRtspURLType_RecordDownload = 2    //¼������
};

//#define    WriteHIKPsPacketData       1 //�Ƿ�д����PS��

#define    RtspServerRecvDataLength             1024*32      //�����и�����Ĵ�С 
#define    MaxRtpSendAudioMediaBufferLength     1024*8       //����ƴ��RTP����׼������ 
#define    VideoStartTimestampFlag              0xEFEFEFEF   //��Ƶ��ʼʱ��� 

class CNetClientRecvRtsp : public CNetRevcBase
{
public:
	CNetClientRecvRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
   ~CNetClientRecvRtsp() ;

   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
   virtual int ProcessNetData();

   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//������Ƶ����
   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//������Ƶ����

   virtual int SendVideo() ;//������Ƶ����
   virtual int SendAudio() ;//������Ƶ����
   virtual int SendFirstRequst();//���͵�һ������
   virtual bool RequestM3u8File();//����m3u8�ļ�

   mpeg4_aac_t     aacInfo;
   ts_demuxer_t    *ts=NULL;

   bool           bUniviewFlag;//�������������ͷ������Ҫ�ж���Ƶ�����ʽ
   bool           destroy();
   char           m_szContentBaseURL[string_length_1024];//���Describe ������� Content-Base �ֶΣ���ʹ�ø�URL
   int            nRecvRtpPacketCount;
   unsigned short nMaxRtpLength;
   uint64_t       nSendOptionsHeartbeatTimer;
   void           SendOptionsHeartbeat();

   uint32_t     cbVideoTimestamp;//�ص�ʱ���
   uint32_t     cbVideoLength;//�ص���Ƶ�ۼ�
   bool         RtspPause();
   bool         RtspResume();
   bool         RtspSpeed(char* nSpeed);
   bool         RtspSeek(char* szSeekTime);
   WWW_AuthenticateType       m_wwwType;
   unsigned char              szCallBackAudio[2048];
   unsigned char*             szCallBackVideo;

   _rtp_header                rtpHead;

   bool                       FindRtpPacketFlag();
   bool                       StartRtpPsDemux();

#ifdef           WriteHIKPsPacketData
   FILE*          fWritePS;
#endif

   uint32_t                   psHandle; 

   int                        nRtspProcessStep;
   int                        nTrackIDOrer;
   int                        CSeq;
   unsigned int               nMediaCount;
   unsigned int               nSendSetupCount;
   char                       szWww_authenticate[string_length_2048];//ժҪ��֤�������ɷ��������͹�����
   WWW_AuthenticateType       AuthenticateType;//rtsp��ʲô������֤
   char                       szBasic[string_length_2048];//����rtsp������֤
   char                       szSessionID[string_length_2048];//sessionID 
   char                       szTrackIDArray[16][string_length_1024];

   bool  GetWWW_Authenticate();
   bool  getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce);
   void  SendPlay(WWW_AuthenticateType wwwType);
   void  SendSetup(WWW_AuthenticateType wwwType);
   void  SendDescribe(WWW_AuthenticateType wwwType);
   void  SendOptions(WWW_AuthenticateType wwwType);
   void  UserPasswordBase64(char* szUserPwdBase64);
   bool  FindVideoAudioInSDP();

   unsigned char           s_extra_data[string_length_2048];
   int                     extra_data_size;
   struct mpeg4_avc_t      avc;
   bool                     bStartWriteFlag ;

   int                     nSendRtpFailCount;//�ۼƷ���rtp��ʧ�ܴ��� 

   bool                    GetSPSPPSFromDescribeSDP();

   int64_t                 nCurrentTime;
   int                     videoSSRC;
   bool                    bSendRRReportFlag;
   int                     audioSSRC;
   CRtcpPacketSR           rtcpSR;
   CRtcpPacketRR           rtcpRR;
   unsigned char           szRtcpSRBuffer[string_length_2048];
   unsigned int            rtcpSRBufferLength;
   unsigned char           szRtcpDataOverTCP[string_length_2048];
   void                    SendRtcpReportData();//����rtcp �����,���Ͷ�
   void                    SendRtcpReportDataRR(unsigned int nSSRC, int nChan);//����rtcp �����,���ն�
   void                    ProcessRtcpData(char* szRtpData, int nDataLength, int nChan);

   int                     GetRtspPathCount(char* szRtspURL);//ͳ��rtsp URL ·������

   volatile                uint64_t tRtspProcessStartTime; //��ʼʱ��

   std::mutex              MediaSumRtpMutex;
   unsigned short          nVideoRtpLen, nAudioRtpLen;

   unsigned char            szRtpDataOverTCP[string_length_2048];
   unsigned char            szAudioRtpDataOverTCP[string_length_2048];

   uint32_t                hRtpVideo, hRtpAudio;
   uint32_t                nVideoSSRC;

   char                    szRtspSDPContent[string_length_4096];
   char                    szRtspAudioSDP[string_length_4096];

   bool                    GetMediaInfoFromRtspSDP();
   void                    SplitterRtpAACData(unsigned char* rtpAAC, int nLength);
   int32_t                 XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain);
   bool                    ReadRtspEnd();

   int                     au_header_length;
   uint8_t                 *ptr, *pau, *pend;
   int                     au_size ; // only AU-size
   int                     au_numbers ;
   int                     SplitterSize[16];

   std::mutex              netDataLock;
   unsigned char           netDataCache[MaxNetDataCacheBufferLength]; //�������ݻ���
   int                     netDataCacheLength;//�������ݻ����С
   int                     nNetStart, nNetEnd; //����������ʼλ��\����λ��
   int                     MaxNetDataCacheCount;

   unsigned char           data_[RtspServerRecvDataLength];//ÿһ֡rtsp���ݣ�����rtsp �� rtp �� 
   unsigned int            data_Length;
   unsigned short          nRtpLength;
   int                     nContentLength;

   RtspProtect      RtspProtectArray[MaxRtspProtectCount];
   int              RtspProtectArrayOrder;
 
   int             FindHttpHeadEndFlag();
   int             FindKeyValueFlag(char* szData);
   void            GetHttpModemHttpURL(char* szMedomHttpURL);
   int             FillHttpHeadToStruct();
   bool            GetFieldValue(char* szFieldName, char* szFieldValue);

   bool            bReadHeadCompleteFlag; //�Ƿ��ȡ���HTTPͷ
   int             nRecvLength;           //�Ѿ���ȡ��ϵĳ���
   unsigned char   szHttpHeadEndFlag[8];  //Httpͷ������־
   int             nHttpHeadEndLength;    //Httpͷ������־��ĳ��� 
   char            szResponseHttpHead[string_length_2048];
   char            szCSeq[string_length_2048];
   char            szTransport[string_length_2048];

   char            szResponseBuffer[string_length_4096];
   int             nSendRet;
  static   uint64_t Session ;
  uint64_t         currentSession;
  char             szCurRtspURL[string_length_2048];
  int64_t           nPrintCount;

   //ֻ����rtsp������� OPTIONS,DESCRIBE,SETUP,PALY 
   void            InputRtspData(unsigned char* pRecvData, int nDataLength);

   void           AddADTSHeadToAAC(unsigned char* szData, int nAACLength);
   unsigned char  aacData[4096];
   int            timeValue;
   struct rtp_payload_t   hRtpHandle[MaxRtpHandleCount];
   void*                  rtpDecoder[MaxRtpHandleCount];
   char           szSdpAudioName[string_length_1024];
   char           szVideoName[string_length_1024];
   char           szAudioName[string_length_1024];
   int            nVideoPayload;
   int            nAudioPayload;
   int            sample_index;//����Ƶ������Ӧ����� 
   int            nChannels; //��Ƶͨ����
   int            nSampleRate; //��Ƶ����Ƶ��
   char           szRtspContentSDP[string_length_4096];
   char           szVideoSDP[string_length_4096];
   char           szAudioSDP[string_length_4096];
   CABLSipParse   sipParseV, sipParseA;   //sdp ��Ϣ����

#ifdef USE_BOOST

   boost::shared_ptr<CMediaStreamSource> pMediaSource;
#else
   std::shared_ptr<CMediaStreamSource> pMediaSource;
#endif

   volatile bool  bIsInvalidConnectFlag; //�Ƿ�Ϊ�Ƿ����� 
   volatile bool  bExitProcessFlagArray[3];

   FILE*          fWriteRtpVideo;
   FILE*          fWriteRtpAudio;
   FILE*          fWriteESStream;
};

#endif