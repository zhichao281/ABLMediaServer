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





#endif
	//�������ң�������Ѿ�����
	CNetRevcBase_ptr GetNetRevcBaseClientNoLock(NETHANDLE CltHandle);
	bool                                  DeleteNetRevcBaseClient(NETHANDLE CltHandle);
	bool                                  DeleteMediaStreamSource(char* szURL);
	bool                                  DeleteClientMediaStreamSource(uint64_t nClient);
	;
	// ��ȡ�̳߳�ʵ��
	static CGlobalManager& getInstance();
private:

	~CGlobalManager() {};

	CGlobalManager(const CGlobalManager&) = delete;

	CGlobalManager& operator=(const CGlobalManager&) = delete;

	MediaServerPort                       ABL_MediaServerPort;
	std::mutex                                                       ABL_CNetRevcBase_ptrMapLock;
	CNetRevcBase_ptrMap                                              xh_ABLNetRevcBaseMap;

	CMediaFifo                                                       pDisconnectBaseNetFifo;             //������ѵ����� 
	CMediaFifo                                                       pReConnectStreamProxyFifo;          //��Ҫ�������Ӵ���ID 
	CMediaFifo                                                       pMessageNoticeFifo;          //��Ϣ֪ͨFIFO

};

