//#ifndef _NetServerSendWebRTC_H
//#define _NetServerSendWebRTC_H
//
//#ifdef USE_BOOST
//
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/smart_ptr/shared_ptr.hpp>
//#include <boost/unordered/unordered_map.hpp>
//#include <boost/make_shared.hpp>
//#include <boost/algorithm/string.hpp>
//
//using namespace boost;
//#else
//
//#endif
//
//
//#include <nice/agent.h>
//#include <gio/gnetworking.h>
//#include <openssl/ssl.h>
//#include <openssl/dtls1.h>
//#include <openssl/err.h>
//#include "srtp2/srtp.h"
//
//#include "rtp_packet.h"
//
//#define  VIDEO_PALYLOAD_H264        103
//#define  VIDEO_LEVEL_ID_H264        "42e01f"
//#define  VIDEO_PALYLOAD_H265        109
//#define  VIDEO_LEVEL_ID_H265        "42e01f"
//#define  AUDIO_PALYLOAD_AAC         10
//#define  AUDIO_PALYLOAD_G722        9
//#define  AUDIO_PALYLOAD_PCMU        0
//#define  AUDIO_PALYLOAD_PCMA        8
//#define	 DTLS_FINGER_PRINT		   "82:7F:45:F6:A1:B8:CB:5A:48:A2:EA:C8:BF:A5:54:E9:61:14:6E:A4:4E:9B:70:67:6B:B4:B5:2C:C6:14:A0:D5"
//
//enum WebRTC_Comm_State
//{
//	WebRTC_Comm_State_NoConnecting   = 0,//尚未连接
//	WebRTC_Comm_State_ConnectSuccess = 1,//连接成功
//	WebRTC_Comm_State_ResponseGet     = 2,//响应GET请求 
//	WebRTC_Comm_State_ResponseWebpage = 3,//回复网页给浏览器
//	WebRTC_Comm_State_ResponseOPTIONS = 4,//回复OPTIONS  
//	WebRTC_Comm_State_ResponsePOST    = 5,//收到post命令回复
//	WebRTC_Comm_State_SendPATCH       = 6,//发送完毕PATCH命令 
//	WebRTC_Comm_State_ResponsePATCH   = 7,//收到PATCH命令回复
//
//	WebRTC_Comm_State_StartPlay       = 9,//开始webrtc发送码流播放 
//	WebRTC_Comm_State_Delete          = 10,//收到PATCH命令回复
//	
//};
//
//
//#define Send_ResponseHttp_MaxPacketCount   1024*48  //回复http包最大发送一次字节
//
//#define  MaxNetClientHttpBuffer        1024*512
//
//#define  MaxClientRespnseInfoLength    1024*512   
//
////#define  WriteVidedDataFlag            1
////#define WriteRtpPacketFlag               1 
//
//class CNetServerSendWebRTC : public CNetRevcBase
//{
//public:
//	CNetServerSendWebRTC(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
//   ~CNetServerSendWebRTC() ;
//
//   virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address) ;
//   virtual int ProcessNetData();
//
//   virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec) ;//塞入视频数据
//   virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate) ;//塞入音频数据
//   virtual int SendVideo();//发送视频数据
//   virtual int SendAudio();//发送音频数据
//   virtual int SendFirstRequst();//发送第一个请求
//   virtual bool RequestM3u8File();//请求m3u8文件
//
//#ifdef  WriteRtpPacketFlag
//   _rtsp_header rtspHead ;   
//   FILE*  writeRtpFile ;
//#endif   
//#ifdef WriteVidedDataFlag
//   FILE*  writeVideoFile ;
//#endif  
//
//   uint32_t                nVdeoFrameNumber ;
//   uint32_t                nAudioFrameNumber ;
//
//   void                    deleteWebRTC();
//   bool                    GetVideoPayload(char* videoCodecName) ;
//   unsigned char           srtpData[string_length_4096];
//   int                     srtLength ;
//   _rtp_packet_sessionopt  optionVideo;
//   _rtp_packet_input       inputVideo;
//   _rtp_packet_sessionopt  optionAudio;
//   _rtp_packet_input       inputAudio;
//   uint32_t                hRtpVideo, hRtpAudio;   
//   int                     nVideoPayload;
//   int                     nAudioPayload;
//   
//    srtp_t          m_srtp;
//    static SSL_CTX* sm_dtlsCtx;
//    SSL*            m_ssl;
//  	BIO*            m_inBio;
//	BIO*            m_outBio;
//	char            m_bioBuff[8192]; 
//	void            cleanDtls();
//	SRTP_PROTECTION_PROFILE m_srtpProf;
//	unsigned char   m_srtpKey[512]={0};
//	unsigned char   m_srvSrtpKey[512]={0};
//	int64_t         m_succPlayTT;
//
//  	std::string     ip;
//	int             port;
//	uint64_t        priority;
//	static int	    m_nVideoSSRC;
//	int             m_nAudioRate, m_nAudioChannel; 
// 	std::string     m_localUfrag;
//	std::string     m_localPwd;  
//	int32_t         m_localCandidateFlag;
//	uint32_t        m_vStreamId;
//	uint32_t        m_aStreamId;
//	std::string     generateUfrag();
//	std::string     generatePwd();
//	std::string     m_clientSdp;
//	std::string     m_location;
//	
//   int64_t                 currentTimestampMs();
//   bool                    SrtpInit();
//   void                    cleanICE();
//   int32_t                 DoDtls();
//   void                    cleanSrtp();
//   static GMainLoop*       getMainLoop();
//   static GMainContext*    getMainContext();
//   NiceAgent*              m_iceAgent;
//   void                    ResponseHttpMessageToClient(bool bSuccess);
//   std::string             XHServerCreateSdp(std::string localUfrag, std::string localPwd, int videoId, std::string levelStrId, std::string ip, int port, uint64_t priority,std::string fingerprint, int ssrc,int nAudioRate,int nAudioChannel);
//   static void             onCandidateGatheringDone(NiceAgent* agent, guint streamId, gpointer data);
//   static void             onNewSelectedPair(NiceAgent* agent, guint streamId, guint componentId, gchar* lfoundation, gchar* rfoundation, gpointer data);
//   static void             onComponentStateChanged(NiceAgent* agent, guint streamId, guint componentId, guint state, gpointer data);
//   static void             onNiceRecv(NiceAgent* agent, guint streamId, guint componentId, guint len, gchar* buf, gpointer data);
//   
//   void                    handleCandidateGatheringDone(guint streamId);
//   void                    handleNewSelectedPair(guint streamId, guint componentId, gchar* lfoundation, gchar* rfoundation);
//   void                    handleComponentStateChanged(guint streamId, guint componentId, guint state);
//   void                    handleNiceRecv(guint streamId, guint componentId, guint len, gchar* buf);
// 
//   char                    szHttpModem[string_length_256] = {0};
//   char                    szHttpURL[string_length_1024] = {0} ;
//   void                    DeleteAllHttpKeyValue();
//   bool                    GetKeyValue(char* key, char* value);
//   bool                    SplitterTextParam(char* szTextParam);
//
//   char                    webrtcPlayerFile[string_length_512];
//   int                     webrtcPlayerFileLength;
//   NETHANDLE               nWebRtcUDP[2];
//   char                    szEtag[1024 * 16];
//   bool                    ResponsePost();
//   bool                    ResponseOPTIONS();
//   bool                    ResponseWebpage();
//   bool                    SendPatchRequestData();
//   bool                    ResponseGetRqeuset();
//   int                     CheckHttpHeadEnd();
//
//   char                    szRequestData[16*1024];
//   char                    szResponseURL[1024 * 16];//支持用户自定义的url 
//
//   RequestKeyValueMap      requestKeyValueMap;
//   CABLSipParse            httpParse;
//   std::mutex              NetClientHTTPLock;
//   unsigned char           netDataCache[MaxNetClientHttpBuffer]; //网络数据缓存
//    int                    nHttpHeadEndLength;
//   int                     netDataCacheLength;//网络数据缓存大小
//   int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
//   char                    szHttpHead[1024 * 64];
//   char                    szHttpBody[1024 * 64];
//   char                    szContentLength[256] = { 0 };
//   int                     nContent_Length = 0;
//   char                    szHttpPath[1024 * 64];
//   char                    szConnection[1024 * 16];
//};
//
//typedef std::shared_ptr<CNetServerSendWebRTC> CNetServerSendWebRTC_ptr;
//
//#endif