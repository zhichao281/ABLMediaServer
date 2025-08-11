// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef  _Stdafx_H
#define  _Stdafx_H

//NETHANDLE�̶�Ϊuint64_t��������arm������������⣬��Ϊ�� uint32_t 
#define NETHANDLE uint64_t

//���嵱ǰ����ϵͳΪWindows 
#if (defined _WIN32 || defined _WIN64)
 #define      OS_System_Windows        1
#endif

#ifdef OS_System_Windows


#include <stdio.h>
#include <tchar.h>

#include <WinSock2.h>
#include <Windows.h>
#include <objbase.h>  
#include <iphlpapi.h>

#include <thread>
#include <mutex>
#include <map>
#include <list>
#include <vector>
#include <string.h>
#include <malloc.h>

#include "cudaCodecDLL.h"

#else 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>
#include <thread>
#include <fcntl.h>
#include <dirent.h>

#include<sys/types.h> 
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h> 
#include <ifaddrs.h>
#include <netdb.h>

#include <pthread.h>
#include <signal.h>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <vector>
#include <math.h>
#include <iconv.h>
#include <malloc.h>
#include <dlfcn.h> 
#include "cudaCodecDLL.h"
#include "cudaEncodeDLL.h"

#include <limits.h>
#include <sys/resource.h>

#define      BYTE     unsigned char 

#endif

//�����ַ������� 
#define  string_length_256    256  
#define  string_length_512    512 
#define  string_length_1024   1024  
#define  string_length_2048   2048  
#define  string_length_4096   4096 
#define  string_length_8192   8192 
#define  string_length_48K    1024*48 
#define  string_length_512K   1024*512 

uint64_t GetCurrentSecond();
uint64_t GetCurrentSecondByTime(char* szDateTime);
bool     QureyRecordFileFromRecordSource(char* szShareURL, char* szFileName);

//ws_socket ͬѧ״̬ 
enum WebSocketCommStatus
{
	WebSocketCommStatus_NoConnect = 0, //û������
	WebSocketCommStatus_Connect = 1,//�Ѿ�����
	WebSocketCommStatus_ShakeHands = 2,//��ʼ����
	WebSocketCommStatus_HandsSuccess = 3 //websocket ���ӳɹ�
};

//rtsp ��������
enum ABLRtspPlayerType
{
	RtspPlayerType_Unknow = -1 ,  //δ֪ 
	RtspPlayerType_Liveing = 0 , //ʵʱ����
	RtspPlayerType_RecordReplay = 1 ,//¼��ط�
	RtspPlayerType_RecordDownload = 2,//¼������
};

//ý���ʽ��Ϣ
struct MediaCodecInfo
{
	char szVideoName[64]; //H264��H265 
	int  nVideoFrameRate; //��Ƶ֡�ٶ� 
	int  nWidth;          //��Ƶ��
	int  nHeight;         //��Ƶ��
	int  nVideoBitrate;   //��Ƶ����

	char szAudioName[64]; //AAC�� G711A�� G711U
	int  nChannels;       //ͨ������ 1 ��2 
	int  nSampleRate;     //����Ƶ�� 8000��16000��22050 ��32000�� 44100 �� 48000 
	int  nBaseAddAudioTimeStamp;//ÿ����Ƶ����
	int  nAudioBitrate;   //��Ƶ����

	MediaCodecInfo()
	{
		memset(szVideoName, 0x00, sizeof(szVideoName));
		nVideoFrameRate = 25 ;
		memset(szAudioName, 0x00, sizeof(szAudioName));
		nChannels = 0;
		nSampleRate = 0;
		nBaseAddAudioTimeStamp = 0 ;
		nWidth = 0;
		nHeight = 0;
		nVideoBitrate = 0;
		nAudioBitrate = 0;
	};
};

//ý����������ж˿ڽṹ
struct MediaServerPort
{
	char secret[string_length_256];  //api��������
	uint64_t nServerStartTime;//����������ʱ��
	bool     bNoticeStartEvent;//�Ƿ��Ѿ�֪ͨ���� 
	int  nHttpServerPort; //http����˿�
	int  nRtspPort;     //rtsp
	int  nRtmpPort;     //rtmp
	int  nHttpFlvPort;  //http-flv
	int  nWSFlvPort;    //ws-flv
	int  nHttpMp4Port;  //http-mp4
	int  nWebRtcPort;  //webrtc 
	int  ps_tsRecvPort; //���굥�˿�
	int  WsRecvPcmPort;//˽��Э�����pcm��Ƶ
	int  n1078Port;//1078���˿ڽ�������
	int  jtt1078Version;//1078���˿ڽ��������İ汾��

	int  nHlsPort;     //Hls �˿� 
	int  nHlsEnable;   //HLS �Ƿ��� 
	int  nHLSCutType;  //HLS��Ƭ��ʽ  1 Ӳ�̣�2 �ڴ� 
	int  nH265CutType; //H265��Ƭ��ʽ 1  ��ƬΪTS ��2 ��ƬΪ mp4  
	int  hlsCutTime; //��Ƭʱ��
	int  nMaxTsFileCount;//�������TS��Ƭ�ļ�����
	char wwwPath[256];//hls��Ƭ·��

	int  nRecvThreadCount;//�����������ݽ����߳����� 
	int  nSendThreadCount;//�����������ݷ����߳�����
	int  nRecordReplayThread;//����¼��طţ���ȡ�ļ����߳�����
	int  nGBRtpTCPHeadType;  //GB28181 TCP ��ʽ����rtp(����PS)����ʱ����ͷ����ѡ��1�� 4���ֽڷ�ʽ��2��2���ֽڷ�ʽ��
	int  nEnableAudio;//�Ƿ�������Ƶ

	int  nIOContentNumber; //ioContent����
	int  nThreadCountOfIOContent;//ÿ��iocontent�ϴ������߳�����
	int  nReConnectingCount;//��������

	char recordPath[512];//¼�񱣴�·��
	int  pushEnable_mp4;//���������Ƿ���¼��
	int  fileSecond;//fmp4�и�ʱ��
	int  videoFileFormat;//¼���ļ���ʽ 1 Ϊ fmp4, 2 Ϊ mp4 
	int  fileKeepMaxTime;//¼���ļ������ʱ������λСʱ
	int  recordFileCutType; //�и�һ������¼���ļ��ķ�ʽ
	int  enable_GetFileDuration;//��ѯ¼���б��Ƿ��ȡ¼���ļ�����ʱ��
	int  httpDownloadSpeed;//http¼�������ٶ��趨
	int  fileRepeat;//MP4�㲥(rtsp/rtmp/http-flv/ws-flv)�Ƿ�ѭ�������ļ�

	char picturePath[256];//ͼƬץ�ı���·��
	int  pictureMaxCount; //ÿ·ý��Դ���ץ�ı�������
	int  snapOutPictureWidth;//ץ�������
	int  snapOutPictureHeight;//ץ�������
	int  captureReplayType; //ץ�ķ�������
	int  snapObjectDestroy;//ץ�Ķ����Ƿ�����
	int  snapObjectDuration;//ץ�Ķ��������ʱ������λ��
	int  deleteSnapPicture;//�Ƿ�ɾ��ץ��ͼƬ 
	int  maxSameTimeSnap;//ץ����󲢷�����
	float  maxTimeNoOneWatch;//���˹ۿ����ʱ�� 
	int  nG711ConvertAAC; //�Ƿ�ת��ΪAAC 
	char ABL_szLocalIP[128];
	char mediaServerID[256];

	int  H265ConvertH264_enable;
	int  H265DecodeCpuGpuType;
	int  convertOutWidth;
	int	 convertOutHeight;
	int  convertMaxObject;
	int  convertOutBitrate;
	int  H264DecodeEncode_enable;
	int  filterVideo_enable;
	char filterVideoText[1280];
	int  nFilterFontSize;
	char  nFilterFontColor[64];
	float nFilterFontAlpha;//͸����
	int  nFilterFontLeft;//x����
	int  nFilterFontTop;//y����

	//�¼�֪ͨģ��
	int  hook_enable;//�Ƿ����¼�֪ͨ
	int  noneReaderDuration;//���˹ۿ�ʱ�䳤
	char on_stream_arrive[string_length_512];
	char on_stream_not_arrive[string_length_512]; //����δ���� ���������������������֧�� 
	char on_stream_none_reader[string_length_512];
	char on_stream_disconnect[string_length_512];
	char on_stream_not_found[string_length_512];
	char on_record_mp4[string_length_512];
	char on_record_progress[string_length_512];//¼�����
	char on_record_ts[string_length_512];
	char on_server_started[string_length_512];
	char on_server_keepalive[string_length_512];
	char on_delete_record_mp4[string_length_512];
	char on_stream_iframe_arrive[string_length_512];
	char on_play[string_length_512];
	char on_publish[string_length_512];
	char on_rtsp_replay[string_length_512];

	uint64_t    nClientNoneReader;
	uint64_t    nClientNotFound;
	uint64_t    nClientRecordMp4;
	uint64_t    nClientDeleteRecordMp4;
	uint64_t    nClientRecordProgress;
	uint64_t    nClientArrive;
	uint64_t    nClientNotArrive;
	uint64_t    nClientDisconnect;
	uint64_t    nClientRecordTS;
	uint64_t    nServerStarted;//����������
	uint64_t    nServerKeepalive;//������������Ϣ 
	uint64_t    nPlay;//����
	uint64_t    nPublish;//����
	uint64_t    nFrameArrive;//I֡���
	uint64_t    nRtspReplay;//rtsp¼��ط�
	int         MaxDiconnectTimeoutSecond;//�����߳�ʱ���
	int         ForceSendingIFrame;//ǿ�Ʒ���I֡ 
	uint64_t    nServerKeepaliveTime;//����������ʱ��

	char       debugPath[string_length_512];//�����ļ�
	int        nSaveProxyRtspRtp;//�Ƿ񱣴������������0 �����棬1 ����
	int        nSaveGB28181Rtp;//�Ƿ񱣴�GB28181���ݣ�0 δ���棬1 ���� 

	int        gb28181LibraryUse;//��������������ѡ��, 1 ʹ�����п����������⣬2 ʹ�ñ����ϳ¹���������� 
	uint64_t   iframeArriveNoticCount;//I֪֡ͨ����
	int        httqRequstClose;//�Ƿ�Ϊ�����Ӳ��� 
	int        keepaliveDuration; //��������ʱ����
	int        flvPlayAddMute;
	int        GB28181RtpMinPort;
	int        GB28181RtpMaxPort;
	int        nSyncWritePacket;//�����������ݰ���ʽ  

	int         nUseWvp = 0; //�Ƿ�ο�wvp-zlm�Ľӿڷ���  Ϊ1ʱ�򷵻ظ�ʽ��ZLM��һ��
	char port_range[string_length_512]; //����˿ڷ�Χ������ȷ��36���˿�
	char listeningip[string_length_512]; //webrtc��������ip
	char externalip[string_length_512]; //webrtc��������ip
	int listeningport;    //webrtc���������˿�
	int minport;    //UDP�м̶˿ڷ�Χ������UDPת����ע�ⰲȫ���ͨ
	int maxport;    //UDP�м̶˿ڷ�Χ������UDPת����ע�ⰲȫ���ͨ
	char realm[string_length_512]; //Ĭ����Realm
	char user[string_length_512]; //��ݵ�����û���ʹ��user=XXX:XXXX�ķ�ʽ

	MediaServerPort()
	{
		memset(wwwPath, 0x00, sizeof(wwwPath));
		nServerStartTime = 0;
		bNoticeStartEvent = false;
		memset(on_server_started, 0x00, sizeof(on_server_started));
		memset(on_server_keepalive, 0x00, sizeof(on_server_keepalive));
		memset(on_delete_record_mp4, 0x00, sizeof(on_delete_record_mp4));
		memset(secret, 0x00, sizeof(secret));
		nRtspPort    = 554;
		nRtmpPort    = 1935;
		nHttpFlvPort = 8088;
		nHttpMp4Port = 8089;
		WsRecvPcmPort = 9298;
		ps_tsRecvPort = 10000;

		nHlsPort     = 9088;
		nHlsEnable   = 0;
		nHLSCutType  = 1;
		nH265CutType = 1;
		hlsCutTime = 3;
		nMaxTsFileCount = 20;

		nRecvThreadCount = 64;
		nSendThreadCount = 64;
		nRecordReplayThread = 64;
		nGBRtpTCPHeadType = 1;
		nEnableAudio = 0 ;

		nIOContentNumber = 16;
		nThreadCountOfIOContent = 16;

		memset(recordPath, 0x00, sizeof(recordPath));
		fileSecond = 180;
		videoFileFormat = 1;
		pushEnable_mp4 = 0;
		fileKeepMaxTime = 12;
		httpDownloadSpeed = 6;
		fileRepeat = 0;
		maxTimeNoOneWatch = 1;
		nG711ConvertAAC = 0;
		memset(mediaServerID, 0x00, sizeof(mediaServerID));
		memset(picturePath, 0x00, sizeof(picturePath));
		pictureMaxCount = 10;
		captureReplayType = 1;
		snapObjectDestroy = 1;
		snapObjectDuration = 120;
		deleteSnapPicture = 0;
		memset(ABL_szLocalIP, 0x00, sizeof(ABL_szLocalIP));

		hook_enable = 0;
		noneReaderDuration = 30;
		memset(on_stream_none_reader, 0x00, sizeof(on_stream_none_reader));
		memset(on_stream_not_found, 0x00, sizeof(on_stream_not_found));
		memset(on_record_mp4, 0x00, sizeof(on_record_mp4));
		memset(on_record_progress, 0x00, sizeof(on_record_progress));
		memset(on_stream_arrive, 0x00, sizeof(on_stream_arrive));
		memset(on_stream_not_arrive, 0x00, sizeof(on_stream_not_arrive));
 		memset(on_record_ts, 0x00, sizeof(on_record_ts));
		memset(on_stream_disconnect, 0x00, sizeof(on_stream_disconnect));
		memset(on_play, 0x00, sizeof(on_play));
		memset(on_publish, 0x00, sizeof(on_publish));
		memset(on_stream_iframe_arrive, 0x00, sizeof(on_stream_iframe_arrive));
		memset(on_rtsp_replay, 0x00, sizeof(on_rtsp_replay));
 
		nClientNoneReader = 0 ;
		nClientNotFound = 0;
		nClientRecordMp4 = 0;
		nClientDeleteRecordMp4 = 0;
		nClientRecordProgress = 0;
		nClientArrive = 0;
		nClientNotArrive = 0;
		nClientDisconnect = 0;
		nClientRecordTS = 0;
		nServerStarted = 0;
		nServerKeepalive = 0;
		nPlay=0;//����
	    nPublish=0;//����
		nFrameArrive = 0;

		maxSameTimeSnap = 16;
		snapOutPictureWidth; 
		snapOutPictureHeight; 

		H265ConvertH264_enable = 0;
		H265DecodeCpuGpuType = 0;
		convertOutWidth = 720;
		convertOutHeight = 576;
		convertMaxObject = 24;
		convertOutBitrate = 512;
		H264DecodeEncode_enable=0;
		filterVideo_enable=0;
		memset(filterVideoText,0x00,sizeof(filterVideoText));
		nFilterFontSize=12;
		memset(nFilterFontColor,0x00,sizeof(nFilterFontColor));
		nFilterFontAlpha = 0.6;//͸����
		nFilterFontLeft = 10;//x����
		nFilterFontTop = 10 ;//y����
		MaxDiconnectTimeoutSecond = 16;
		ForceSendingIFrame = 0;

		nSaveProxyRtspRtp = 0; 
		nSaveGB28181Rtp = 0 ; 
		memset(debugPath, 0x00, sizeof(debugPath));

		gb28181LibraryUse = 1;
		iframeArriveNoticCount = 30;
		httqRequstClose = 0;
		enable_GetFileDuration = 0;
		keepaliveDuration = 20;
		flvPlayAddMute = 1;
		GB28181RtpMinPort = 35000;
		GB28181RtpMaxPort = 40000;
		nSyncWritePacket = 0;
		n1078Port = 1078;
		jtt1078Version = 2016;

 	}
};

//��Ե���ĳһ·��Ƶת��ṹ
struct H265ConvertH264Struct
{
	int  H265ConvertH264_enable;//H265�Ƿ�ת��
	int  H264DecodeEncode_enable;//h264�Ƿ����½��룬�ٱ��� 
 	int  convertOutWidth;
	int	 convertOutHeight;
 	int  convertOutBitrate;

	H265ConvertH264Struct()
	{
	  H265ConvertH264_enable = 0;
	  H264DecodeEncode_enable = 0;
 	  convertOutWidth = 0;
	  convertOutHeight = 0;
 	  convertOutBitrate = 512;
 	}
};

//�����������
enum NetBaseNetType
{
	NetBaseNetType_Unknown                 = 20 ,//δ�������������
	NetBaseNetType_RtspProtectBaseState    = 15 ,//rtsp����Э���ʼ״̬ 
	NetBaseNetType_RtspServerRecvPushVideo = 16 ,//����rtsp����udp��ʽ����Ƶ
	NetBaseNetType_RtspServerRecvPushAudio = 17 ,//����rtsp����udp��ʽ����Ƶ
	NetBaseNetType_GB28181TcpPSInputStream = 18,//ͨ��10000�˿�TCP��ʽ���չ���PS������
	NetBaseNetType_WebSocektRecvAudio      = 19 ,//ͨ��websocet ������Ƶ
	NetBaseNetType_RtmpServerRecvPush      = 21,//RTMP �����������տͻ��˵����� 
	NetBaseNetType_RtmpServerSendPush      = 22,//RTMP ��������ת���ͻ��˵�������������
	NetBaseNetType_RtspServerRecvPush      = 23,//RTSP �����������տͻ��˵����� 
	NetBaseNetType_RtspServerSendPush      = 24,//RTSP ��������ת���ͻ��˵�������������
	NetBaseNetType_HttpFLVServerSendPush   = 25,//Http-FLV ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_HttpHLSServerSendPush   = 26,//Http-HLS ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_WsFLVServerSendPush     = 27,//WS-FLV ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������
	NetBaseNetType_HttpMP4ServerSendPush   = 28,//http-mp4 ��������ת�� rtsp ��rtmp ��GB28181�ȵ�����������������mp4���ͳ�ȥ
	NetBaseNetType_WebRtcServerWhepPlayer  = 29,//WebRtc ��������ת�� rtsp ��rtmp ��GB28181�ȵ���������������

	//������������
	NetBaseNetType_RtspClientRecv          = 30 ,//rtsp������������ 
	NetBaseNetType_RtmpClientRecv          = 31 ,//rtmp������������ 
	NetBaseNetType_HttpFlvClientRecv       = 32 ,//http-flv������������ 
	NetBaseNetType_HttpHLSClientRecv       = 33 ,//http-hls������������ 
	NetBaseNetType_HttClientRecvJTT1078    = 34, //���ս�ͨ��JTT1078 
	NetBaseNetType_ReadLocalMediaFile      = 35, //�����ȡ����ý���ļ���Ҫ����mp4�ļ�
	NetBaseNetType_FFmpegRecvNetworkMedia  = 36,//rtsp ,rtmp ,http-flv ,http-mp4 ,http-hls ������ 

	//������������
	NetBaseNetType_RtspClientPush          = 40,//rtsp������������ 
	NetBaseNetType_RtmpClientPush          = 41,//rtmp������������ 
 	NetBaseNetType_GB28181ClientPushTCP    = 42,//GB28181������������ 
	NetBaseNetType_GB28181ClientPushUDP    = 43,//GB28181������������ 

	NetBaseNetType_addStreamProxyControl   = 50,//���ƴ���rtsp\rtmp\flv\hsl ����
	NetBaseNetType_addPushProxyControl     = 51,//���ƴ���rtsp\rtmp  ���� ����

	NetBaseNetType_NetGB28181RtpServerListen      = 56,//����TCP��ʽ����Listen��
	NetBaseNetType_NetGB28181RtpSendListen        = 57,//����TCP��ʽ����Listen��
	NetBaseNetType_NetGB28181RtpServerTCP_Active  = 58,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181SendRtpTCP_Passive   = 59, //����28181 TCP��ʽ ��������,������ʽ��������

	NetBaseNetType_NetGB28181RtpServerUDP         = 60,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181RtpServerTCP_Server  = 61,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181RtpServerTCP_Client  = 62,//����28181 TCP��ʽ ��������,�������ӷ�ʽ 
	NetBaseNetType_NetGB28181RtpServerRTCP        = 63,//����28181 UDP��ʽ �������� �е� rtcp ��
	NetBaseNetType_NetGB28181UDPPSStreamInput     = 64,//PS����������굥�˿���������
	NetBaseNetType_NetGB28181SendRtpUDP           = 65,//����28181 UDP��ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Connect   = 66,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������
	NetBaseNetType_NetGB28181SendRtpTCP_Server    = 67,//����28181 TCP��ʽ ��������,�������ӷ�ʽ ��������
	NetBaseNetType_NetGB28181RecvRtpPS_TS         = 68,//����28181 ���˿ڽ���PS��TS����
	NetBaseNetType_NetGB28181UDPTSStreamInput     = 69,//TS��������

	NetBaseNetType_RecordFile_FMP4                = 70,//¼��洢Ϊfmp4��ʽ
	NetBaseNetType_RecordFile_TS                  = 71,//¼��洢ΪTS��ʽ
	NetBaseNetType_RecordFile_PS                  = 72,//¼��洢ΪPS��ʽ
	NetBaseNetType_RecordFile_FLV                 = 73,//¼��洢Ϊflv��ʽ
	NetBaseNetType_RecordFile_MP4                 = 74,//¼��洢Ϊmp4��ʽ

 	ReadRecordFileInput_ReadFMP4File              = 80,//�Զ�ȡfmp4�ļ���ʽ 
	ReadRecordFileInput_ReadTSFile                = 81,//�Զ�ȡTS�ļ���ʽ 
	ReadRecordFileInput_ReadPSFile                = 82,//�Զ�ȡPS�ļ���ʽ 
	ReadRecordFileInput_ReadFLVFile               = 83,//�Զ�ȡFLV�ļ���ʽ 

	NetBaseNetType_HttpClient_None_reader           = 90,//���˹ۿ�
	NetBaseNetType_HttpClient_Not_found             = 91,//��û���ҵ�
	NetBaseNetType_HttpClient_Record_mp4            = 92,//���һ��¼��
	NetBaseNetType_HttpClient_on_stream_arrive      = 93,//��������
	NetBaseNetType_HttpClient_on_stream_disconnect  = 94,//�����ѶϿ�
	NetBaseNetType_HttpClient_on_record_ts          = 95,//TS��Ƭ���
	NetBaseNetType_HttpClient_on_stream_not_arrive  = 96,//����û�е���
	NetBaseNetType_HttpClient_Record_Progress       = 97,//¼�����ؽ���
	NetBaseNetType_HttpClient_on_rtsp_replay        = 98,//rtsp¼��ط��¼�֪ͨ

	NetBaseNetType_SnapPicture_JPEG               =100,//ץ��ΪJPG 
	NetBaseNetType_SnapPicture_PNG                =101,//ץ��ΪPNG

	NetBaseNetType_NetServerHTTP                  =110,//http �������� 

	NetBaseNetType_HttpClient_ServerStarted       = 120,//����������
	NetBaseNetType_HttpClient_ServerKeepalive     = 121,//����������
	NetBaseNetType_HttpClient_DeleteRecordMp4     = 122,//����¼���ļ�
	NetBaseNetType_HttpClient_on_play              = 123,//������Ƶ�¼�֪ͨ
	NetBaseNetType_HttpClient_on_publish           = 124,//��������֪ͨ 
	NetBaseNetType_HttpClient_on_iframe_arrive     = 125,//i֡�����¼�

	NetBaseNetType_NetClientWebrtcPlayer           = 130,//webrtc�Ĳ��� 
	NetBaseNetType_NetServerReadMultRecordFile     = 140,//������ȡ���¼���ļ�
};

#define   MediaServerVerson                 "ABLMediaServer-6.3.6(2025-06-19)"
#define   RtspServerPublic                  "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD��GET_PARAMETER"
#define   RecordFileReplaySplitter          "__ReplayFMP4RecordFile__"  //ʵ����¼�����ֵı�־�ַ�������������ʵ����������url�С�

#define  MaxNetDataCacheBufferLength        1024*1024*3   //���绺�������ֽڴ�С
#define  MaxLiveingVideoFifoBufferLength    1024*1024*3   //������Ƶ���� 
#define  MaxLiveingAudioFifoBufferLength    1024*512      //������Ƶ���� 
#define  BaseRecvRtpSSRCNumber              0xFFFFFFFFFF  //���ڽ���TS����ʱ ���� ssrc ��ֵ��Ϊ�ؼ��� Key
#define  IDRFrameMaxBufferLength            1024*1024*3   //IDR֡��󻺴������ֽڴ�С
#define  MaxClientConnectTimerout           10*1000       //���ӷ��������ʱʱ�� 10 �� 
#define  OpenMp4FileToReadWaitMaxMilliSecond    20        //��mp4�ļ���50����� �ſ�ʼ��ȡ�ļ� 
#define  MaxWriteRecordCacheFFLushLength    1024*1024*3   //ÿ�λ���4M�ֽڵݽ�һ��Ӳ�� 

//rtsp url ��ַ�ֽ�
//rtsp://admin:szga2019@190.15.240.189:554
//rtsp ://190.16.37.52:554/03067970000000000102?DstCode=01&ServiceType=1&ClientType=1&StreamID=1&SrcTP=2&DstTP=2&SrcPP=1&DstPP=1&MediaTransMode=0&BroadcastType=0&Token=jCqM1pVyGb6stUfpLZDvgBG92nGzNBbP&DomainCode=49b5dca295cf42b283ca1d5dd2a0f398&UserId=8&
struct RtspURLParseStruct
{
	char szNoPasswordRtspUrl[string_length_1024]; //û�������url 
	char szSrcRtspPullUrl[string_length_1024]; //ԭʼURL
	char szDstRtspUrl[string_length_1024];//����ַ���RTSP url 
	char szRequestFile[string_length_1024];//������ļ� ���� http://admin:szga2019@190.15.240.189:9088/Media/Camera_00001/hls.m3u8 �е� /Media/Camera_00001/hls.m3u8
	                                  // ���� http://admin:szga2019@190.15.240.189:8088/Media/Camera_00001.flv �е� /Media/Camera_00001.flv
	char szRtspURLTrim[string_length_1024];//ȥ����������ַ����õ���rtsp url 

	bool bHavePassword; //�Ƿ�������
	char szUser[128]; //�û���
	char szPwd[128];//����
	char szIP[128]; //IP
	char szPort[128];//�˿�

	char szRealm[128];//������֤����
	char szNonce[128];//������֤����

	RtspURLParseStruct()
	{
		memset(szNoPasswordRtspUrl, 0x00, sizeof(szNoPasswordRtspUrl));
		memset(szSrcRtspPullUrl, 0x00, sizeof(szSrcRtspPullUrl));
		memset(szDstRtspUrl, 0x00, sizeof(szDstRtspUrl));
		memset(szRequestFile, 0x00, sizeof(szRequestFile));
		memset(szRtspURLTrim, 0x00, sizeof(szRtspURLTrim));

		memset(szUser, 0x00, sizeof(szUser));
		memset(szPwd, 0x00, sizeof(szPwd));
		memset(szIP, 0x00, sizeof(szIP));
		memset(szPort, 0x00, sizeof(szPort));
		memset(szRealm, 0x00, sizeof(szRealm));
		memset(szNonce, 0x00, sizeof(szNonce));
		bHavePassword = false;
	}
};

//��������ת�������ṹ
struct addStreamProxyStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 
	char  url[string_length_1024];//������ַ ��֧�� rtsp\rtmp\http-flv \ hls 
	char  isRtspRecordURL[64];//�Ƿ�rtsp¼��ط� 
	char  enable_mp4[64];//�Ƿ�¼��
	char  enable_hls[64];//�Ƿ���hls
	char  convertOutWidth[64];//ת�������
	char  convertOutHeight[64];//ת������� 
	char  H264DecodeEncode_enable[64];//H264�Ƿ�����ٱ��� 
	char  RtpPayloadDataType[64]; 
	char  disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char  disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   dst_url[string_length_256];//TCP����ʱ Ŀ��IP 
	char   dst_port[string_length_256];//TCP����ʱ Ŀ��˿� 
	char   optionsHeartbeat[64];//�Ƿ���options������Ϊ������
	char   fileKeepMaxTime[128];//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 
	char   videoFileFormat[64];//¼���ļ���ʽ  1  fmp4(ps), 2  mp4 , 3  ts  ��addStreamProxy ��openRtpServer ���������޸� 
	char   G711ConvertAAC[64];//g711a\g711u �Ƿ�ת��Ϊ aac ,Ĭ�ϴ������ļ���ȡ

	addStreamProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
		memset(isRtspRecordURL, 0x00, sizeof(isRtspRecordURL));
		memset(enable_mp4, 0x00, sizeof(enable_mp4));
		memset(enable_hls, 0x00, sizeof(enable_hls));
		memset(convertOutWidth, 0x00, sizeof(convertOutWidth));
		memset(convertOutHeight, 0x00, sizeof(convertOutHeight));
		memset(H264DecodeEncode_enable, 0x00, sizeof(H264DecodeEncode_enable));
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
		memset(disableVideo, 0x00, sizeof(disableVideo));
		memset(disableAudio, 0x00, sizeof(disableAudio));
		memset(dst_url, 0x00, sizeof(dst_url));
		memset(dst_port, 0x00, sizeof(dst_port));
		memset(optionsHeartbeat, 0x00, sizeof(optionsHeartbeat));
		memset(fileKeepMaxTime, 0x00, sizeof(fileKeepMaxTime));
		memset(videoFileFormat, 0x00, sizeof(videoFileFormat));
		strcpy(videoFileFormat, "1");
		strcpy(G711ConvertAAC, "1");
	}
};

//����ɾ������ṹ
struct delRequestStruct
{
	char  secret[string_length_256];//api�������� 
	char  key[128];//key
 
	delRequestStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
	}
};

//��������ת�������ṹ
struct addPushProxyStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 
	char  url[string_length_1024];//������ַ ��֧�� rtsp\rtmp\http-flv \ hls 
	char  disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char  disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 

	addPushProxyStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(url, 0x00, sizeof(url));
		memset(disableVideo, 0x00, sizeof(disableVideo));
		memset(disableAudio, 0x00, sizeof(disableAudio));
	}
};

//����GB28181��������
struct openRtpServerStruct
{
	char   secret[string_length_256];//api�������� 
	char   vhost[string_length_256];//���������������
	char   app[string_length_256];//�������Ӧ����
	char   stream_id[string_length_512];//�������id 
	char   port[64] ;//GB2818�˿�
	char   port2[64];//����ES������2�˿� 
	char   enable_tcp[16]; //0 UDP��1 TCP ������2 TCP ���� 
	char   payload[64]; //payload rtp �����payload 
	char   dst_url[string_length_256];//TCP����ʱ Ŀ��IP 
	char   dst_port[string_length_256];//TCP����ʱ Ŀ��˿� 
	char   enable_mp4[64];//�Ƿ�¼��
	char   enable_hls[64];//�Ƿ���hls
	char   convertOutWidth[64];//ת�������
	char   convertOutHeight[64];//ת������� 
	char   H264DecodeEncode_enable[64];//H264�Ƿ�����ٱ��� 
	char   RtpPayloadDataType[16];//�������rtp���ص��������� �� 1 rtp + PS �� , �� 2 rtp + ES �� ,��3 ��rtp + XHB һ�ҹ�˾��˽�и�ʽ��
	char   disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   send_app[string_length_256];//���ͳ�ȥ
	char   send_stream_id[string_length_512];//���ͳ�ȥ
	char   send_disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   send_disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   jtt1078_version[128]; //1078�汾 2013��2016��2019
	char   detectSendAppStream[64];//���ظ��������Ƿ����
	char   fileKeepMaxTime[128];//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 
	char   videoFileFormat[64];//¼���ļ���ʽ  1  fmp4(ps), 2  mp4 , 3  ts  ��addStreamProxy ��openRtpServer ���������޸� 
	char   jtt1078_KeepOpenPortType[64];//jtt1078�����˿����� 0 ���ǳ����˿� ��Ĭ�ϣ�ֻ����1�Σ��豸�رոö˿ڹرա� ��1  ����Ϊʵʱ��Ƶ  �� 2 ����Ϊ¼��ط� �� 3  ����Ϊ�����Խ� ��4 Ϊ���� ����������˿�
	char   G711ConvertAAC[64];//g711a\g711u �Ƿ�ת��Ϊ aac ,Ĭ�ϴ������ļ���ȡ

	openRtpServerStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream_id, 0x00, sizeof(stream_id));
		memset(port, 0x00, sizeof(port));
		memset(enable_tcp, 0x00, sizeof(enable_tcp));
		memset(payload, 0x00, sizeof(payload));
		memset(enable_mp4, 0x00, sizeof(enable_mp4));
		memset(enable_hls, 0x00, sizeof(enable_hls));
		memset(convertOutWidth, 0x00, sizeof(convertOutWidth));
		memset(convertOutHeight, 0x00, sizeof(convertOutHeight));
		memset(H264DecodeEncode_enable, 0x00, sizeof(H264DecodeEncode_enable));
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
		memset(disableVideo, 0x00, sizeof(disableVideo));
		memset(disableAudio, 0x00, sizeof(disableAudio));
		memset(send_app, 0x00, sizeof(send_app));
		memset(send_stream_id, 0x00, sizeof(send_stream_id));
		memset(send_disableVideo, 0x00, sizeof(send_disableVideo));
		memset(send_disableAudio, 0x00, sizeof(send_disableAudio));
		memset(dst_url, 0x00, sizeof(dst_url));
		memset(dst_port, 0x00, sizeof(dst_port));
		memset(jtt1078_version, 0x00, sizeof(jtt1078_version));
		memset(detectSendAppStream, 0x00, sizeof(detectSendAppStream));
		memset(fileKeepMaxTime, 0x00, sizeof(fileKeepMaxTime));
		memset(port2, 0x00, sizeof(port2));
		memset(videoFileFormat, 0x00, sizeof(videoFileFormat));
		memset(jtt1078_KeepOpenPortType, 0x00, sizeof(jtt1078_KeepOpenPortType));
		strcpy(jtt1078_KeepOpenPortType, "0");
		strcpy(G711ConvertAAC, "1");
	}
};

//����GB28181��������
struct startSendRtpStruct
{
	char   secret[string_length_256];//api�������� 
	char   vhost[string_length_256];//���������������
	char   app[string_length_256];//�������Ӧ����
	char   stream[string_length_512];//�������id 
	char   ssrc[128];//ssrc
	char   src_port[64];//����Դ�󶨵Ķ˿ںţ�0 �Զ�����һ���˿ڣ�����0 ����û�ָ���Ķ˿�
	char   dst_url[512];//
	char   dst_port[64];//GB2818�˿�
	char   is_udp[16]; //0 UDP��1 TCP 
	char   payload[24]; //payload rtp �����payload 
	char   RtpPayloadDataType[64];//�����ʽ 1��PS������ES������XHB
	char   disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   recv_app[string_length_256];//���ڽ���
	char   recv_stream[string_length_512];//���ڽ���
	char   recv_disableVideo[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   recv_disableAudio[16];//���˵���Ƶ 1 ���˵���Ƶ ��0 ��������Ƶ ��Ĭ�� 0 
	char   jtt1078_version[128]; //1078�汾 2013��2016��2019
	char   enable_hls[16];//����������Ƿ���hls
	char   enable_mp4[16];//����������Ƿ񱣴�MP4 
	char   fileKeepMaxTime[128];//¼����󱣴�ʱ�� ����ȷ��ÿһ·ý��Դ���ȴ������ļ���ȡĬ��ֵ��addStreamProxy ��openRtpServer ���������޸� 

	startSendRtpStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(ssrc, 0x00, sizeof(ssrc));
		memset(dst_url, 0x00, sizeof(dst_url));
		memset(dst_port, 0x00, sizeof(dst_port));
		memset(is_udp, 0x00, sizeof(is_udp));
		memset(payload, 0x00, sizeof(payload));
		memset(RtpPayloadDataType, 0x00, sizeof(RtpPayloadDataType));
		memset(disableVideo, 0x00, sizeof(disableVideo));
		memset(disableAudio, 0x00, sizeof(disableAudio));
		memset(recv_app, 0x00, sizeof(recv_app));
		memset(recv_stream, 0x00, sizeof(recv_stream));
		memset(recv_disableVideo, 0x00, sizeof(recv_disableVideo));
		memset(recv_disableAudio, 0x00, sizeof(recv_disableAudio));
		memset(jtt1078_version, 0x00, sizeof(jtt1078_version));
		memset(enable_hls, 0x00, sizeof(enable_hls));
		memset(enable_mp4, 0x00, sizeof(enable_mp4));
		memset(fileKeepMaxTime, 0x00, sizeof(fileKeepMaxTime));
	}
};

//��ȡ�б�ṹ
struct getMediaListStruct 
{
	char   secret[string_length_256];//api�������� 
	char   vhost[string_length_256];//���������������
	char   app[string_length_256];//�������Ӧ����
	char   stream[string_length_512];//�������id 

	getMediaListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
	}
};

//��ȡ���ⷢ��ý���б�ṹ
struct getOutListStruct
{
	char   secret[string_length_256];//api�������� 
	char   outType[128];//ý��Դ���� rtsp ,rtmp ,flv ,hls ,gb28181 ,webrtc  
	getOutListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(outType, 0x00, sizeof(outType));
	}
};

//��ȡϵͳ����
struct getServerConfigStruct
{
	char   secret[string_length_256];//api�������� 
	getServerConfigStruct()
	{
		memset(secret, 0x00, sizeof(secret));
 	}
};

//�ر�Դ���ṹ
struct closeStreamsStruct
{
	char    secret[string_length_256];//api��������
	char    schema[string_length_256];
	char   	vhost[string_length_256];
	char   	app[string_length_256];
	char   	stream[string_length_512];
	int	    force ; //1 ǿ�ƹرգ������Ƿ����˹ۿ� ��0 �����ܹۿ�ʱ�����ر�
	closeStreamsStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(schema, 0x00, sizeof(schema));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		force = 1; 
	}
};

//��ʼ��ֹͣ¼��
struct startStopRecordStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 

	startStopRecordStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
 	}
};

//��ѯ¼���ļ��б�
struct queryRecordListStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 
	char  starttime[128];//��ʼʱ��
	char  endtime[128];//��ʼʱ��

	queryRecordListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(starttime, 0x00, sizeof(starttime));
		memset(endtime, 0x00, sizeof(endtime));
	}
};

//����ץ��
struct getSnapStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 
	char  timeout_sec[128];//ץ��ͼƬ��ʱ ����λ ��
	char  captureReplayType[64];//ץ�ķ�������

	getSnapStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(timeout_sec, 0x00, sizeof(timeout_sec));
		memset(captureReplayType, 0x00, sizeof(captureReplayType));
	}
};

//��ѯͼƬ�ļ��б�
struct queryPictureListStruct
{
	char  secret[string_length_256];//api�������� 
	char  vhost[string_length_256];//���������������
	char  app[string_length_256];//�������Ӧ����
	char  stream[string_length_512];//�������id 
	char  starttime[128];//��ʼʱ�� 
	char  endtime[128];//��ʼʱ��  

	queryPictureListStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(starttime, 0x00, sizeof(starttime));
		memset(endtime, 0x00, sizeof(endtime));
	}
};

//��ѯͼƬ�ļ��б�
struct controlStreamProxy
{
	char  secret[string_length_256];//api�������� 
	char  key[64];//key
 	char  command[string_length_256];//���� 
	char  value[string_length_256];//ֵ

	controlStreamProxy()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
		memset(command, 0x00, sizeof(command));
		memset(value, 0x00, sizeof(value));
 	}
};

//�������ò���ֵ
struct SetConfigParamValue
{
	char  secret[string_length_256];//api�������� 
	char  key[string_length_256];//key
	char  value[string_length_512];//ֵ

	SetConfigParamValue()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
 		memset(value, 0x00, sizeof(value));
	}
};

//��ȡ������ռ�ö˿ڽṹ
struct ListServerPortStruct
{
	char   secret[string_length_256];//api�������� 
	char   vhost[string_length_256];//���������������
	char   app[string_length_256];//�������Ӧ����
	char   stream[string_length_512];//�������id 

	ListServerPortStruct()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
	}
};

//���������ͣ������ pauseResumeRtpServer 
struct pauseResumeRtpServer
{
	char  secret[string_length_256];//api�������� 
	char  key[128];//key

	pauseResumeRtpServer()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(key, 0x00, sizeof(key));
	}
};

//��ȡ������ռ�ö˿ڽṹ
struct sendJtt1078Talk
{
	char   secret[string_length_256];//api�������� 
	char   vhost[string_length_256];//���������������
	char   app[string_length_256];//�������Ӧ����
	char   stream[string_length_512];//�������id 
	char   send_app[string_length_256];//��1078�豸���͵���Ƶ�� app
	char   send_stream_id[string_length_512];//��1078�豸���͵���Ƶ�� stream_id

	sendJtt1078Talk()
	{
		memset(secret, 0x00, sizeof(secret));
		memset(vhost, 0x00, sizeof(vhost));
		memset(app, 0x00, sizeof(app));
		memset(stream, 0x00, sizeof(stream));
		memset(send_app, 0x00, sizeof(send_app));
		memset(send_stream_id, 0x00, sizeof(send_stream_id));
	}
};

enum NetRevcBaseClientType
{
	NetRevcBaseClient_ServerAccept               = 1, //�������˿ڽ��� ������ 554,8080,8088,8089,1935 �ȵȶ˿�accept������
	NetRevcBaseClient_addStreamProxy             = 2, //����������ʽ
	NetRevcBaseClient_addPushStreamProxy         = 3, //����������ʽ
	NetRevcBaseClient_addStreamProxyControl      = 4, //���ƴ�������
	NetRevcBaseClient_addPushProxyControl        = 5, //���ƴ�������
	NetRevcBaseClient__NetGB28181Proxy           = 6, //GB28181���� 
	NetRevcBaseClient_addFFmpegProxyControl      = 10 ,//ffmpeg ������������
	NetRevcBaseClient_addFFmpegProxy             = 11, //����������ʽ
};

//http������� 
struct RequestKeyValue
{
	char key[512];
	char value[1280];
	RequestKeyValue()
	{
		memset(key, 0x00, sizeof(key));
		memset(value, 0x00, sizeof(value));
	}
};

//http ����������
enum HttpReponseIndexApiCode
{
	IndexApiCode_OK           = 0 ,     //��������
	IndexApiCode_ErrorRequest = -100,   //�Ƿ�����
	IndexApiCode_secretError  = -200,   //secret����
	IndexApiCode_ParamError   = -300,   //��������
	IndexApiCode_KeyNotFound  = -400,   //Key û���ҵ�
	IndexApiCode_SqlError     = -500,   //Sql����
	IndexApiCode_ConnectFail  = -600,   //����ʧ��
	IndexApiCode_RtspSDPError = -700,   //rtsp����ʧ��
	IndexApiCode_RtmpPushError = -800,  //rtmp����ʧ��
	IndexApiCode_BindPortError = -900,  //�����ʧ��
	IndexApiCode_ConnectTimeout = -1000, //�������ӳ�ʱ
	IndexApiCode_HttpJsonError = -1001, //http ���� json�����Ƿ�
	IndexApiCode_HttpProtocolError = -1002, //http ���� Э�����
	IndexApiCode_MediaSourceNotFound = -1003, //����ý��Դû���ҵ� 
	IndexApiCode_RequestProcessFailed  = -1004 ,//ִ��ʧ�� 
	IndexApiCode_RequestFileNotFound = -1005,//�ļ�û���ҵ�
	IndexApiCode_ContentTypeNotSupported = -1006,//Content-Type ���Ͳ�֧��
	IndexApiCode_OverMaxSameTimeSnap = -1007,//�������ץ������
	IndexApiCode_AppStreamAlreadyUsed = -1008,//app/sream �Ѿ�ʹ����
	IndexApiCode_PortAlreadyUsed      = -1009,//port �Ѿ�ʹ����
	IndexApiCode_SSRClreadyUsed       = -1010,//SSRC �Ѿ�ʹ����
	IndexApiCode_TranscodingVideoFilterNotEnable = -1011,//ת�롢ˮӡ������δ����
	IndexApiCode_TranscodingVideoFilterTakeEffect = -1012,//ת�롢ˮӡ������δ��Ч
	IndexApiCode_dst_url_dst_portUsed = -1013,//�Ѿ���ͬһ��Ŀ���ַ���˿ڷ��͹�
	IndexApiCode_RecvRtmpFailed = -1100, //��ȡrtmp����ʧ��
	IndexApiCode_AppStreamHaveUsing = -1200, //app,stream ����ʹ�ã�����������δ���� 
};

//rtsp�������� DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD
enum RtspProcessStep
{
	RtspProcessStep_Initing = 0,//rtsp �ոճ�ʼ��
	RtspProcessStep_OPTIONS = 1, //������� OPTIONS
	RtspProcessStep_ANNOUNCE = 2,//������� ANNOUNCE
	RtspProcessStep_DESCRIBE = 3,//������� DESCRIBE
	RtspProcessStep_SETUP = 4,//������� SETUP
	RtspProcessStep_RECORD = 5,//������� RECORD 
	RtspProcessStep_PLAY = 6,//ֻ�Ƿ�����Play���� 
	RtspProcessStep_PLAYSucess = 7,//������� PLAY
	RtspProcessStep_PAUSE = 8,//������� PAUSE
	RtspProcessStep_TEARDOWN = 9 //������� TEARDOWN
};
//rtsp��֤��ʽ 
enum  WWW_AuthenticateType
{
	WWW_Authenticate_UnKnow = -1, //��δȷ����ʽ
	WWW_Authenticate_None = 0,  //����֤
	WWW_Authenticate_MD5 = 1,  //MD5��֤��ժҪ��֤
	WWW_Authenticate_Basic = 2    //base 64 ������֤
};

//����������� 
enum NetGB28181ProxyType
{
	NetGB28181ProxyType_RecvStream = 1, //��������
	NetGB28181ProxyType_PushStream = 2 ,//��������
};

//accept ��������������
enum NetServerHandleType
{
	NetServerHandleType_GB28181RecvStream = 10, //tcp��ʽ���չ�������
};

//����
struct NetServerHandleParam
{
	int      nNetServerHandleType;
	uint64_t hParent; //�����Ķ��� 
	char     szMediaSource[256];
	int      nAcceptNumber; //���Ӵ��� 
	NetServerHandleParam()
	{
		nNetServerHandleType = 0;
		hParent = 0;
		memset(szMediaSource, 0x00, sizeof(szMediaSource));
		nAcceptNumber = 0 ;
	}
};

//rtp��ͷ
struct _rtp_header
{
	uint8_t cc : 4;
	uint8_t x : 1;
	uint8_t p : 1;
	uint8_t v : 2;
	uint8_t payload : 7;
	uint8_t mark : 1;
	uint16_t seq;
	uint32_t timestamp;
	uint32_t ssrc;

	_rtp_header()
		: v(2), p(0), x(0), cc(0)
		, mark(0), payload(0)
		, seq(0)
		, timestamp(0)
		, ssrc(0)
	{
	}
};

struct _rtsp_header
{
	unsigned char  head;
	unsigned char  chan;
	unsigned short Length;
};

//ý��Դ����
enum MediaSourceType
{
	MediaSourceType_LiveMedia = 0,  //ʵ������
	MediaSourceType_ReplayMedia = 1, //¼��㲥
};

//rtsp ����ʱ rtp ���ص��������� 
enum RtspRtpPayloadType
{
	RtspRtpPayloadType_Unknow = 0,  //δ֪
	RtspRtpPayloadType_ES     = 1, //rtp����ES 
	RtspRtpPayloadType_PS     = 2, //rtp����PS 
    RtspRtpPayloadType_TS     = 3, //rtp����TS 
};

//ͼƬ����
enum HttpImageType
{
	HttpImageType_jpeg = 1,  //jpeg
    HttpImageType_png = 2, //png
};
bool          ABLDeleteFile(char* szFileName);

//��Ϣ֪ͨ�ṹ 
struct MessageNoticeStruct
{
	uint64_t nClient;
	char     szMsg[string_length_4096];
	int      nSendCount;
	MessageNoticeStruct()
	{
		nClient = 0;
		nSendCount = 0;
		memset(szMsg, 0x00, sizeof(szMsg));
	}
};

//rtsp �� rtp ���紫������
enum RtspNetworkType
{
	RtspNetworkType_Unknow = -1, //δ֪
	RtspNetworkType_TCP = 1,//TCP
	RtspNetworkType_UDP = 2,//UDP 
};

#define  MaxGB28181RtpSendVideoMediaBufferLength  1024*64 
#define  Ma1078CacheBufferLength                  1024*1024*2   //1078���� 
#define  MaxOne1078PacketLength                   950           //1078���1������ 

#pragma pack(push)
#pragma pack (1)
typedef struct Jt1078VideoRtpPacket_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[6];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	int64_t timestamp;
	unsigned short i_frame_interval;
	unsigned short frame_interval;
	unsigned short payload_size;

}Jt1078VideoRtpPacket;

typedef struct Jt1078VideoRtpPacket2019_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[10];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	int64_t timestamp;
	unsigned short i_frame_interval;
	unsigned short frame_interval;
	unsigned short payload_size;

}Jt1078VideoRtpPacket2019;

typedef struct Jt1078AudioRtpPacket_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[6];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	int64_t timestamp;
	unsigned short payload_size;

}Jt1078AudioRtpPacket;

typedef struct Jt1078AudioRtpPacket2019_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[10];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	int64_t timestamp;
	unsigned short payload_size;

}Jt1078AudioRtpPacket2019;

typedef struct Jt1078OtherRtpPacket_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[6];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	unsigned short payload_size;

}Jt1078OtherRtpPacket;

typedef struct Jt1078OtherRtpPacket2019_T {

	unsigned char head[4];
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char v : 2;
	unsigned char p : 1;
	unsigned char x : 1;
	unsigned char cc : 4;
	unsigned char m : 1;
	unsigned char pt : 7;
#else
	unsigned char cc : 4;
	unsigned char x : 1;
	unsigned char p : 1;
	unsigned char v : 2;
	unsigned char pt : 7;
	unsigned char m : 1;
#endif
	unsigned short seq;
	unsigned char sim[10];
	unsigned char ch;
#ifdef IS_JT1078RTP_BIGENDIAN
	unsigned char frame_type : 4;
	unsigned char packet_type : 4;
#else
	unsigned char packet_type : 4;
	unsigned char frame_type : 4;
#endif
	unsigned short payload_size;

}Jt1078OtherRtpPacket2019;

#pragma  pack(pop)

//��Ƶaac ��Ƶ���� [1 ͨ��  ��16000 ���� 
struct muteAACBufferStruct
{
	unsigned char    pAACBuffer[256];
	unsigned short   nAACLength;
	muteAACBufferStruct()
	{
		memset(pAACBuffer, 0x00, sizeof(pAACBuffer));
		nAACLength = 0;
	}
};

#ifndef OS_System_Windows
unsigned long GetTickCount();
int64_t  GetTickCount64();
void          Sleep(int mMicroSecond);
#endif

#include "XHNetSDK.h"
#include "ABLSipParse.h"

#ifdef OS_System_Windows 
#include "ABLogSDK.h"

void malloc_trim(int n);

#else 
#include "ABLogFile.h"
#include "SimpleIni.h"
#endif

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/atomic.hpp>
using namespace boost;
#else
#include <atomic>

#include <cctype>

#endif
#include "ABLString.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/bsf.h>
#include <libswscale/swscale.h>
#include <libavutil/base64.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>	
#include <libswresample/swresample.h>
}
using namespace std;

using namespace rapidjson;

typedef list<int> LogFileVector;

#define SAFE_ARRAY_DELETE(x) if( x != NULL ) { delete[] x; x = NULL; }
#define SAFE_RELEASE(x)  if( x != NULL ) { x->Release(); x = NULL; }
#define SAFE_DELETE(x)   if( x != NULL ) { delete x; x = NULL; }

#include "NetRecvBase.h"
#include "NetRtspServerUDP.h"
#include "NetServerHTTP.h"
#include "NetRtspServer.h"
#include "NetRtmpServerRecv.h"
#include "NetServerHTTP_FLV.h"
#include "NetServerWS_FLV.h"
#include "NetServerRecvAudio.h"
#include "NetServerHLS.h"
#include "NetClientHttp.h"
#include "NetClientSnap.h"

#include "ps_demux.h"
#include "ps_mux.h"
#include "MediaFifo.h"
#include "NetBaseThreadPool.h"
#include "FFVideoDecode.h"
#include "FFVideoEncode.h"

#include "MediaStreamSource.h"
#include "rtp_depacket.h"
#include "NetServerRecvRtpTS_PS.h"
#include "RtpTSStreamInput.h"
#include "RtpPSStreamInput.h"

#include "NetClientRecvHttpHLS.h"
#include "NetClientRecvRtmp.h"
#include "NetClientRecvFLV.h"
#include "NetClientRecvRtsp.h"
#include "NetClientSendRtsp.h"
#include "NetClientSendRtmp.h"
#include "NetClientAddStreamProxy.h"
#include "NetClientAddPushProxy.h"
#include "NetGB28181Listen.h"
#include "NetGB28181RtpServer.h"
#include "NetGB28181RtpClient.h"
#include "NetServerHTTP_MP4.h"
#include "StreamRecordFMP4.h"
#include "StreamRecordMP4.h"
#include "StreamRecordTS.h"
#include "RecordFileSource.h"
#include "PictureFileSource.h"
#include "ReadRecordFileInput.h"
#include "LCbase64.h"
#include "SHA1.h"
#include "NetClientReadLocalMediaFile.h"
#include "SimpleIni.h"
#include "NetClientFFmpegRecv.h"
#include "NetServerReadMultRecordFile.h"
#include "NetServerSendWebRTC.h"

#endif
