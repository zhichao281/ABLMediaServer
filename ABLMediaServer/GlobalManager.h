#pragma once
#include "stdafx.h"

#ifdef USE_BOOST
typedef boost::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef boost::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;

#else
typedef std::shared_ptr<CNetRevcBase> CNetRevcBase_ptr;
typedef std::unordered_map<NETHANDLE, CNetRevcBase_ptr>        CNetRevcBase_ptrMap;


#endif //USE_BOOST


class CGlobalManager
{
public:
	CGlobalManager() {};


public:

#ifdef USE_BOOST
	
	 boost::shared_ptr<CNetRevcBase>       GetNetRevcBaseClient(NETHANDLE CltHandle);
	 boost::shared_ptr<CMediaStreamSource> CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
	 boost::shared_ptr<CMediaStreamSource> GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);


#else
	std::shared_ptr<CNetRevcBase>         GetNetRevcBaseClient(NETHANDLE CltHandle);
	std::shared_ptr<CMediaStreamSource>   CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct  h265ConvertH264Struct);
	std::shared_ptr<CMediaStreamSource>   GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound = false);


	//无锁查找，在外层已经有锁
	CNetRevcBase_ptr GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);


#endif
	bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
	bool                                  DeleteMediaStreamSource(char* szURL);
	bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
	;
	// 获取线程池实例
	static CGlobalManager& getInstance();
private:

	~CGlobalManager() {};

	CGlobalManager(const CGlobalManager&) = delete;

	CGlobalManager& operator=(const CGlobalManager&) = delete;

	MediaServerPort                       ABL_MediaServerPort;
	std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
	CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;

	CMediaFifo                                                       pDisconnectBaseNetFifo;             //清理断裂的链接 
	CMediaFifo                                                       pReConnectStreamProxyFifo;          //需要重新连接代理ID 
	CMediaFifo                                                       pMessageNoticeFifo;          //消息通知FIFO

};

