#include "GlobalManager.h"

CGlobalManager& CGlobalManager::getInstance()
{
	static CGlobalManager instance;
	return instance;
  
}

#ifdef USE_BOOST
boost::shared_ptr<CNetRevcBase> CGlobalManager::GetNetRevcBaseClient(NETHANDLE CltHandle)
{
	return boost::shared_ptr<CNetRevcBase>();
}
boost::shared_ptr<CMediaStreamSource> CGlobalManager::CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct h265ConvertH264Struct)
{
	return boost::shared_ptr<CMediaStreamSource>();
}
boost::shared_ptr<CMediaStreamSource> CGlobalManager::GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound)
{
	return boost::shared_ptr<CMediaStreamSource>();
}

#else
std::shared_ptr<CNetRevcBase> CGlobalManager::GetNetRevcBaseClient(NETHANDLE CltHandle)
{
	return std::shared_ptr<CNetRevcBase>();
}
std::shared_ptr<CMediaStreamSource> CGlobalManager::CreateMediaStreamSource(char* szUR, uint64_t nClient, MediaSourceType nSourceType, uint32_t nDuration, H265ConvertH264Struct h265ConvertH264Struct)
{
	return std::shared_ptr<CMediaStreamSource>();
}
std::shared_ptr<CMediaStreamSource> CGlobalManager::GetMediaStreamSource(char* szURL, bool bNoticeStreamNoFound)
{
	return std::shared_ptr<CMediaStreamSource>();
}





#endif //USE_BOOST

bool CGlobalManager::DeleteNetRevcBaseClient(NETHANDLE CltHandle)
{
	std::lock_guard<std::mutex> lock(ABL_CNetRevcBase_ptrMapLock);

	CNetRevcBase_ptrMap::iterator iterator1;

	iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		(*iterator1).second->bRunFlag = false;
		if (((*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientPush || (*iterator1).second->netBaseNetType == NetBaseNetType_RtmpClientPush ||
			(*iterator1).second->netBaseNetType == NetBaseNetType_RtspClientRecv || (*iterator1).second->netBaseNetType == NetBaseNetType_RtmpClientRecv)
			&& (*iterator1).second->bProxySuccessFlag == false)
		{//rtsp\rtmp 代理拉流，rtsp \ rtmp 代理推流
			//如果没有成功过则需要删除父类 
			auto  pParentPtr = GetNetRevcBaseClientNoLock((*iterator1).second->hParent);
			if (pParentPtr && pParentPtr->bProxySuccessFlag == false || (*iterator1).second->m_nXHRtspURLType == XHRtspURLType_RecordPlay)
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->hParent, sizeof((*iterator1).second->hParent));
		}

		//关闭国标监听 
		if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpServerListen)
		{
			XHNetSDK_Unlisten((*iterator1).second->nClient);
			if ((*iterator1).second->nMediaClient == 0)
			{//码流没有达到通知
				if (ABL_MediaServerPort.hook_enable == 1 && ABL_MediaServerPort.nClientNotArrive > 0 && (*iterator1).second->bUpdateVideoFrameSpeedFlag == false)
				{
					MessageNoticeStruct msgNotice;
					msgNotice.nClient = ABL_MediaServerPort.nClientNotArrive;
					sprintf(msgNotice.szMsg, "{\"mediaServerId\":\"%s\",\"app\":\"%s\",\"stream\":\"%s\",\"networkType\":%d,\"key\":%llu}", ABL_MediaServerPort.mediaServerID, (*iterator1).second->m_addStreamProxyStruct.app, (*iterator1).second->m_addStreamProxyStruct.stream, (*iterator1).second->netBaseNetType, (*iterator1).second->nClient);
					pMessageNoticeFifo.push((unsigned char*)&msgNotice, sizeof(MessageNoticeStruct));
				}
			}
			if ((*iterator1).second->nMediaClient > 0)
				pDisconnectBaseNetFifo.push((unsigned char*)&(*iterator1).second->nMediaClient, sizeof((*iterator1).second->nMediaClient));
		}
		else if ((*iterator1).second->netBaseNetType == NetBaseNetType_NetGB28181RtpSendListen)
		{
			XHNetSDK_Unlisten((*iterator1).second->nClient);
			WriteLog(Log_Debug, " XHNetSDK_Unlisten() , nMediaClient = %llu  ", (*iterator1).second->nClient);
		}
		else //在此处关闭，确保boost:asio 单线程状态下 close(pSocket) ;
			XHNetSDK_Disconnect((*iterator1).second->nClient);

		xh_ABLNetRevcBaseMap.erase(iterator1);
		return true;
	}
	else
	{
		return false;
	}
}

//无锁查找，在外层已经有锁
CNetRevcBase_ptr CGlobalManager::GetNetRevcBaseClientNoLock(NETHANDLE CltHandle)
{

	CNetRevcBase_ptr   pClient = NULL;

	auto iterator1 = xh_ABLNetRevcBaseMap.find(CltHandle);
	if (iterator1 != xh_ABLNetRevcBaseMap.end())
	{
		pClient = (*iterator1).second;
		return pClient;
	}
	else
	{
		return NULL;
	}
}
bool CGlobalManager::DeleteMediaStreamSource(char* szURL)
{
	return false;
}

bool CGlobalManager::DeleteClientMediaStreamSource(uint64_t nClient)
{
	return false;
}


