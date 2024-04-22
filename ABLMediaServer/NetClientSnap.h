#ifndef _NetClientSnap_H
#define _NetClientSnap_H

#ifdef USE_BOOST
#include <boost/unordered/unordered_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost;
#else

#endif

#include "VideoDecode.h"


class CNetClientSnap : public CNetRevcBase
{
public:
	CNetClientSnap(NETHANDLE hServer, NETHANDLE hClient, char* szIP, unsigned short nPort, char* szShareMediaURL);
	~CNetClientSnap();

	virtual int InputNetData(NETHANDLE nServerHandle, NETHANDLE nClientHandle, uint8_t* pData, uint32_t nDataLength, void* address);
	virtual int ProcessNetData();

	virtual int PushVideo(uint8_t* pVideoData, uint32_t nDataLength, char* szVideoCodec);//塞入视频数据
	virtual int PushAudio(uint8_t* pVideoData, uint32_t nDataLength, char* szAudioCodec, int nChannels, int SampleRate);//塞入音频数据
	virtual int SendVideo();//发送视频数据
	virtual int SendAudio();//发送音频数据
	virtual int SendFirstRequst();//发送第一个请求
	virtual bool RequestM3u8File();//请求m3u8文件

	volatile bool   bWaitIFrameFlag;
	static  int     nPictureNumber; //同一秒内的序号
	std::mutex      NetClientSnapLock;
	CVideoDecode    videoDecode;
	char            szFileNameOrder[64];
	char            szFileName[string_length_2048];
	bool            bUpdateFlag;
	char            szPictureFileName[string_length_2048];
};

#endif