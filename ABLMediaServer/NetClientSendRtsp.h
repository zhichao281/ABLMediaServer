#ifndef _NetClientSendRtsp_H
#define _NetClientSendRtsp_H

#include "./rtspMD5/DigestAuthentication.hh"

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

#define  MaxRtspValueCount          64 
#define  MaxRtspProtectCount        24 
#define   MaxRtpHandleCount         2

//#define  WriteRtpDepacketFileFlag   1 //�Ƿ�дrtp����ļ�

#define    RtspServerRecvDataLength             1024*64      //�����и�����Ĵ�С 
#define    MaxRtpSendVideoMediaBufferLength     1024*64       //����ƴ��RTP����׼������ 
#define    MaxRtpSendAudioMediaBufferLength     1024*8       //����ƴ��RTP����׼������ 
#define    VideoStartTimestampFlag              0xEFEFEFEF   //��Ƶ��ʼʱ��� 

class CNetClientSendRtsp : public CNetRevcBase
{
public:
	CNetClientSendRtsp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientSendRtsp();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//������Ƶ����
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//������Ƶ����

	virtual int SendVideo();//������Ƶ����
	virtual int SendAudio();//������Ƶ����
	virtual int SendFirstRequst();//���͵�һ������
	virtual bool RequestM3u8File();//����m3u8�ļ�

	int                        nTrackIDOrer;
	int                        CSeq;
	unsigned int               nMediaCount;
	unsigned int               nSendSetupCount;
	char                       szWww_authenticate[string_length_1024];//ժҪ��֤�������ɷ��������͹�����
	WWW_AuthenticateType       AuthenticateType;//rtsp��ʲô������֤
	char                       szBasic[string_length_2048];//����rtsp������֤
	char                       szSessionID[string_length_2048];//sessionID 
	char                       szTrackIDArray[16][512];

	bool  GetWWW_Authenticate();
	bool  getRealmAndNonce(char* szDigestString, char* szRealm, char* szNonce);
	void  SendRECORD(WWW_AuthenticateType wwwType);
	void  SendSetup(WWW_AuthenticateType wwwType);
	void  SendANNOUNCE(WWW_AuthenticateType wwwType);
	void  SendOptions(WWW_AuthenticateType wwwType);
	void  UserPasswordBase64(char* szUserPwdBase64);
	void  FindVideoAudioInSDP();

	unsigned char           s_extra_data[string_length_2048];
	int                     extra_data_size;
	struct mpeg4_avc_t      avc;
#ifdef WriteRtpDepacketFileFlag
	bool                     bStartWriteFlag;
#endif 

	int                     nSendRtpFailCount;//�ۼƷ���rtp��ʧ�ܴ��� 

	bool                    GetSPSPPSFromDescribeSDP();

	CRtcpPacketSR           rtcpSR;
	CRtcpPacketRR           rtcpRR;
	unsigned char           szRtcpSRBuffer[string_length_2048];
	unsigned int            rtcpSRBufferLength;
	unsigned char           szRtcpDataOverTCP[string_length_2048];
	void                    SendRtcpReportData(unsigned int nSSRC, int nChan);//����rtcp �����,���Ͷ�
	void                    SendRtcpReportDataRR(unsigned int nSSRC, int nChan);//����rtcp �����,���ն�
	void                    ProcessRtcpData(char* szRtpData, int nDataLength, int nChan);

	int                     GetRtspPathCount(char* szRtspURL);//ͳ��rtsp URL ·������

	volatile                uint64_t tRtspProcessStartTime; //��ʼʱ��

	unsigned  char          szSendRtpVideoMediaBuffer[MaxRtpSendVideoMediaBufferLength];
	unsigned  char          szSendRtpAudioMediaBuffer[MaxRtpSendAudioMediaBufferLength];
	int                     nSendRtpVideoMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
	int                     nSendRtpAudioMediaBufferLength; //�Ѿ����۵ĳ���  ��Ƶ
	uint32_t                nStartVideoTimestamp; //��һ֡��Ƶ��ʼʱ��� ��
	uint32_t                nCurrentVideoTimestamp;// ��ǰ֡ʱ���
	int                     nCalcAudioFrameCount;//������Ƶ������

	void                    ProcessRtpVideoData(unsigned char* pRtpVideo, int nDataLength);
	void                    ProcessRtpAudioData(unsigned char* pRtpAudio, int nDataLength);
	void                    SumSendRtpMediaBuffer(unsigned char* pRtpMedia, int nRtpLength);//�ۻ�rtp����׼������
	std::mutex              MediaSumRtpMutex;

	unsigned short          nVideoRtpLen, nAudioRtpLen;

	_rtp_packet_sessionopt  optionVideo;
	_rtp_packet_input       inputVideo;
	_rtp_packet_sessionopt  optionAudio;
	_rtp_packet_input       inputAudio;
	uint32_t                hRtpVideo, hRtpAudio;
	uint32_t                nVideoSSRC;

	bool                    GetRtspSDPFromMediaStreamSource(RtspSDPContentStruct sdpContent, bool bGetFlag);
	char                    szRtspSDPContent[string_length_2048];
	char                    szRtspAudioSDP[string_length_2048];

	int32_t                 XHNetSDKRead(NETHANDLE clihandle, uint8_t* buffer, uint32_t* buffsize, uint8_t blocked, uint8_t certain);
	bool                    ReadRtspEnd();

	int                     au_header_length;
	uint8_t* ptr, * pau, * pend;
	int                     au_size; // only AU-size
	int                     au_numbers;
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
	char            szCSeq[128];
	char            szTransport[256];

	char            szResponseBuffer[string_length_4096];
	int             nSendRet;
	static   uint64_t Session;
	uint64_t         currentSession;
	char             szCurRtspURL[string_length_2048];
	int64_t           nPrintCount;

	//ֻ����rtsp������� OPTIONS,DESCRIBE,SETUP,PALY 
	void            InputRtspData(unsigned char* pRecvData, int nDataLength);

	unsigned char  aacData[2048];
	int            timeValue;
	uint32_t       hRtpHandle[MaxRtpHandleCount];
	char           szVideoName[64];
	char           szAudioName[64];
	int            nVideoPayload;
	int            nAudioPayload;
	int            sample_index;//����Ƶ������Ӧ����� 
	int            nChannels; //��Ƶͨ����
	int            nSampleRate; //��Ƶ����Ƶ��
	char           szRtspContentSDP[string_length_2048];
	char           szVideoSDP[string_length_2048];
	char           szAudioSDP[string_length_2048];
	CABLSipParse   sipParseV, sipParseA;   //sdp ��Ϣ����

	volatile bool  bIsInvalidConnectFlag; //�Ƿ�Ϊ�Ƿ����� 
	volatile bool  bExitProcessFlagArray[3];

#ifdef WriteRtpDepacketFileFlag
	FILE* fWriteRtpVideo;
	FILE* fWriteRtpAudio;
#endif
};

#endif