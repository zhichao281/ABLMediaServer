#ifndef _NetClientHttp_H
#define _NetClientHttp_H
#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
#else

#endif

#define Send_ResponseHttp_MaxPacketCount   1024*48  //回复http包最大发送一次字节

#define  MaxNetClientHttpBuffer        1024*1024*1 

#define  MaxClientRespnseInfoLength    1024*1024*1   

class CNetClientHttp : public CNetRevcBase
{
public:
	CNetClientHttp(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientHttp();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据
	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	char                    szResponseData[8192];
	char                    szResponseURL[string_length_2048];//支持用户自定义的url 
	void                    HttpRequest(char* szUrl, char* szBody, int nLength);

	RequestKeyValueMap      requestKeyValueMap;
	CABLSipParse            httpParse;
	std::mutex              NetClientHTTPLock;
	unsigned char           netDataCache[MaxNetClientHttpBuffer]; //网络数据缓存
	int                     netDataCacheLength;//网络数据缓存大小
	int                     nNetStart, nNetEnd; //网络数据起始位置\结束位置
	char                    szHttpHead[1024 * 64];
	char                    szHttpBody[1024 * 64];
	char                    szContentLength[string_length_2048];
	int                     nContent_Length = 0;
	char                    szHttpPath[1024 * 64];
	char                    szConnection[string_length_2048];
};

#endif