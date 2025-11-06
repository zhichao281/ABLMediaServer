/*
功能：
     对音频g711a、g711u,视频 h264、h265、国标gb28181码流进行rtp解包 
*/

#include "stdafx.h"
#include "depacket.h"

const uint8_t NALU_START_CODE[] = { 0x00, 0x00, 0x01 };
const uint8_t SLICE_START_CODE[] = { 0X00, 0X00, 0X00, 0X01 };

depacket::depacket()
{
	cbBuff = NULL;
 	nVpsSpsPpsBufferSize = 0;

	timestamp = 0;
	m_buffsize = 0;
	streamtype = e_rtpdepkt_st_unknow;
}

depacket::~depacket()
{
#ifdef  H265_UseSplitterVideoFlag
	if (cbBuff != NULL)
	{
		delete [] cbBuff;
		cbBuff = NULL;
	}
#endif 
}

bool depacket::CheckIdrFrame(uint8_t* pData, int nLength)
{
	unsigned char frameType = 0;
	bool          bIDRFlag = false;

	for (int i = 0; i < nLength; i++)
	{
		if (memcmp(pData + i, SLICE_START_CODE, sizeof(SLICE_START_CODE)) == 0)
		{
			frameType = pData[4 + i];
			frameType = (frameType & 0x7E) >> 1;

			if (frameType == 19)
			{//IDR 
				bIDRFlag = true;
				break;
			}
		}

		if (i > 512)
			break;
	}

	if (bIDRFlag)
		return true;
	else
		return false;

}

//拷贝 vps sps  pps
bool depacket::CopyVpsSpsPps(uint8_t* pData, int nLength)
{
	if (nVpsSpsPpsBufferSize > 0)
		return false;

	unsigned char frameType = 0;
	bool          bVPSFlag = false;
	bool          bSPSFlag = false;
	bool          bPPSFlag = false;
	bool          bIDRFlag = false;
	int           nVpsPos = 0, nIdrPos = 0;

	for (int i = 0; i < nLength; i++)
	{
		if (memcmp(pData + i, SLICE_START_CODE, sizeof(SLICE_START_CODE)) == 0)
		{
			frameType = pData[4 + i];
			frameType = (frameType & 0x7E) >> 1;
			if (frameType == 32)
			{//VPS 
				nVpsPos = i;
				bVPSFlag = true;
			}
			else if (frameType == 33)
			{//SPS
				bSPSFlag = true;
			}
			else if (frameType == 34)
			{//PPS
				bPPSFlag = true;
			}
			else if (frameType == 19)
			{//IDR 
				nIdrPos = i;
				bIDRFlag = true;
			}

			if (bVPSFlag && bSPSFlag && bPPSFlag && bIDRFlag && (nIdrPos - nVpsPos) > 0 && (nIdrPos - nVpsPos) < DEPACKET_VpsSpsPpsBuffer_SIZE)
			{//拷贝vps ,sps ,pps 
				nVpsSpsPpsBufferSize = nIdrPos - nVpsPos;
				memcpy(szVpsSpsPpsBuffer, pData + nVpsPos, nVpsSpsPpsBufferSize);
				break;
			}
		}
	}

	if (nVpsSpsPpsBufferSize == 0)
	{//VPS、SPS、PPS 单独发送，没有和IDR一起打包
		if (bVPSFlag && bSPSFlag && bPPSFlag && bIDRFlag == false && ((nLength - nVpsPos) <= DEPACKET_VpsSpsPpsBuffer_SIZE))
		{
			nVpsSpsPpsBufferSize = nLength - nVpsPos;
			memcpy(szVpsSpsPpsBuffer, pData + nVpsPos, nVpsSpsPpsBufferSize);
		}
	}

	if (nVpsSpsPpsBufferSize > 0)
		return true;
	else
		return false;
}

//对rtp解包数据进行切割
int32_t depacket::SplitterRtpDepacketBuffer(uint8_t* data, uint32_t datasize)
{
	if (!(streamtype == e_rtpdepkt_st_h265 || streamtype == e_rtpdepkt_st_mpeg4 || streamtype == e_rtpdepkt_st_svacv))
	{//音频数据 或者 h265 直接返回
		if (m_cb)
		{
			cb.data = data;
			cb.datasize = datasize;
			m_cb(&cb);
			m_buffsize = 0;
			return 1;
		}
		return 0;
	}

    //获取vps \ sps \ pps 
	CopyVpsSpsPps(data, datasize);

	//一定要找到VPS、SPS、PPS ，否则解码也是失败 
	if (nVpsSpsPpsBufferSize == 0)
		return -1;

	//查找nalu段
	nPos1 = nPos2 = 0;
	for (int i = 0; i < datasize; i++)
	{
		bFindFrameFlag = false;
		if (memcmp(data + i, SLICE_START_CODE, sizeof(SLICE_START_CODE)) == 0)
		{
			nPos1 = i;
			sclen = sizeof(SLICE_START_CODE);
		}
		else if (memcmp(data + i, NALU_START_CODE, sizeof(NALU_START_CODE)) == 0)
		{
			nPos1 = i;
			sclen = sizeof(NALU_START_CODE);
		}
		else
			continue;

		nFrameType = data[4 + i];
		nFrameType = (nFrameType & 0x7E) >> 1;
		if (i > 0 && (nFrameType == 19 || nFrameType == 1))
		{//判断是否为I、P帧 
			bFullNaluFlag = data[6 + i];
			bFullNaluFlag = (bFullNaluFlag & 0x80) >> 7;
			if (bFullNaluFlag == 1 && ((nFrameType == 19 /*&& checkVpsSpsPps(data, i) == false*/) || nFrameType == 1))
			{//判断为完整的一个 i 帧、或者p帧
				nPos1 = i;
				bFindFrameFlag = true;
				nFindCount++;
			}
		}

		if (bFindFrameFlag && nPos1 > 0 && nPos1 < m_buffsize)
		{
			if (m_cb)
			{//分别返回拼接好的I帧、P帧　
				if (CheckIdrFrame(data, nPos1) == false)
				{
					memcpy(cbBuff, data, nPos1);
					cb.data = cbBuff;
					cb.datasize = nPos1;
				}
				else
				{
					if (nVpsSpsPpsBufferSize > 0)
					{
						memcpy(cbBuff, szVpsSpsPpsBuffer, nVpsSpsPpsBufferSize);
						memcpy(cbBuff + nVpsSpsPpsBufferSize, data, nPos1);
					}
					cb.data = cbBuff;
					cb.datasize = nVpsSpsPpsBufferSize + nPos1;
				}
				m_cb(&cb);
			}

			//剩余的缓存往前移动 
			if((m_buffsize - nPos1) > 0 )
			 memmove(m_buff, m_buff + nPos1, m_buffsize - nPos1);

			m_buffsize = m_buffsize - nPos1;

			datasize = m_buffsize;
			nPos1 = nPos2 = i = 0; //把 i、datasize 的值复位，重新再查找剩余的缓存 
			continue;
		}

		i += sclen;
	}

	return 1;

}


int32_t depacket::handle(unsigned char* pData, int nLength)
{
	if (pData == NULL || nLength <= sizeof(_rtp_header))
		return e_rtpdepkt_err_invaliddata;

	uint32_t  nRet = e_rtpdepkt_err_noerror;
	
	 rtpHead = (_rtp_header*)pData;
	//判断rtp头的版本是否合法
	if(rtpHead->v != 2 )
		return e_rtpdepkt_err_invaliddata; 

	if (rtpHead->payload != payload)
		return e_rtpdepkt_err_invalidpayload;

	//计算扩展头的长度 
	if (rtpHead->x == 1)
		exLength = 4 + ((pData[sizeof(_rtp_header) + 2] << 8) | pData[sizeof(_rtp_header) + 3]) * 4;
	else
		exLength = 0;

	switch (streamtype)
	{
	   case e_rtpdepkt_st_h264:
		   nRet = handle_h264(pData+(sizeof(_rtp_header)+ exLength), nLength - sizeof(_rtp_header) - exLength);
		   break;
	   case e_rtpdepkt_st_h265:
		   nRet = handle_h265(pData + (sizeof(_rtp_header) + exLength), nLength - sizeof(_rtp_header) - exLength);
		   break;
	   default :
		   nRet = handle_common(pData + (sizeof(_rtp_header) + exLength), nLength - sizeof(_rtp_header) - exLength);
 		   break;
	}

	return nRet;
}

//可以支持音频解包、国标PS 解包等等
int32_t depacket::handle_common(unsigned char* pData, int nLength)
{
 	int32_t     ret = e_rtpdepkt_err_noerror;

	//如果时间戳不相同 并且 m_buffsize > 0 (m_buffsize 说明已经拼接过rtp包数据)
	if (::ntohl(rtpHead->timestamp) != timestamp && m_buffsize > 0)
	{
		cb.ssrc     = ::ntohl(rtpHead->ssrc);
		cb.datasize = m_buffsize;
		m_cb(&cb);

		m_buffsize = 0;
	}

	if (RTP_DEPACKET_MAX_SIE - m_buffsize > nLength )
	{
		memcpy(m_buff + m_buffsize, pData , nLength );
		m_buffsize += nLength ;
	}
	else
	{
		m_buffsize = 0;//防止越界
		ret        = e_rtpdepkt_err_invaliddata;
	}

	//更新时间戳
	timestamp = cb.timestamp = ::ntohl(rtpHead->timestamp);
	
	return ret;
}

//对h264进行解包 
int32_t depacket::handle_h264(unsigned char* pData, int nLength)
{
 	int32_t     ret = e_rtpdepkt_err_noerror;
 
	nNalu = pData[0];
	nNalu = nNalu & 0x1F;
 
	//如果时间戳不相同 并且 m_buffsize > 0 (m_buffsize 说明已经拼接过rtp包数据)
	if (::ntohl(rtpHead->timestamp) != timestamp && m_buffsize > 0)
	{
		cb.ssrc = ::ntohl(rtpHead->ssrc);
		cb.datasize = m_buffsize;
		m_cb(&cb);

		m_buffsize = 0;
	}

	if (nNalu >= 0 && nNalu < 24)
	{//单帧打包进行解包
		SingleDepacket(pData, nLength);
	}
	else if (nNalu == 24)
	{//多帧打包 STAP_A 进行解包，h264打包时Nalu头占1个字节，所以还需要rtp头后面往后偏移1字节
		Stap_aDepacket(pData + 1, nLength - 1);
	}
	else if (nNalu == 28)
	{//FU_A 进行解包
		H264FuaDepacket(pData, nLength);
	}
	else//未支持的解包格式
		ret = e_rtpdepkt_err_unsupport;

	//更新时间戳
	timestamp = cb.timestamp = ::ntohl(rtpHead->timestamp);

	return ret;
}

//单帧解包
int32_t depacket::SingleDepacket(unsigned char* pData, int nLength)
{
	int32_t     ret = e_rtpdepkt_err_noerror;
	if (RTP_DEPACKET_MAX_SIE - m_buffsize > nLength )
	{
		memcpy(m_buff + m_buffsize, SLICE_START_CODE, sizeof(SLICE_START_CODE));
		m_buffsize += sizeof(SLICE_START_CODE);

		memcpy(m_buff + m_buffsize, pData , nLength );
		m_buffsize += nLength ;
	}
	else
	{
		m_buffsize = 0;
		ret = e_rtpdepkt_err_bufferfull;
	}

	return ret;
}

//stap_a 解包
int32_t  depacket::Stap_aDepacket(unsigned char* pData, int nLength)
{
	int             nDepacketLength  = nLength;
	int             nPos             = 0 ;
	unsigned short  nFrameLength     = 0 ;//长度是2个字节 
	int32_t nRet                     = e_rtpdepkt_err_noerror;

	while (nDepacketLength > 2)
	{
		memcpy((char*)&nFrameLength, pData + nPos, sizeof(nFrameLength));
		nFrameLength     = ::ntohs(nFrameLength);

		nPos            += sizeof(nFrameLength);//拷贝的地址要移位
		nDepacketLength -= sizeof(nFrameLength);//待解包的长度减少 

		if (nFrameLength > 0 && nFrameLength <= nDepacketLength && (RTP_DEPACKET_MAX_SIE - m_buffsize) >nFrameLength)
		{
			memcpy(m_buff + m_buffsize, SLICE_START_CODE, sizeof(SLICE_START_CODE));
			m_buffsize += sizeof(SLICE_START_CODE);//解包缓存的地址要移位

			memcpy(m_buff + m_buffsize, pData + nPos, nFrameLength);
			nPos            += nFrameLength;//拷贝的地址要移位
			m_buffsize      += nFrameLength;//解包缓存的地址要移位
			nDepacketLength -= nFrameLength;//待解包的长度减少
		}
		else
		{
			nRet = e_rtpdepkt_err_invaliddata;
			break;
		}
	}

	return nRet;
}

//h264 fu_a 解包 
int32_t depacket::H264FuaDepacket(unsigned char* pData, int nLength)
{
    int32_t     ret = e_rtpdepkt_err_noerror;
	
	unsigned char      pH264Nalu = 0;
	_rtp_fua_header*    fua_head = reinterpret_cast<_rtp_fua_header*>(pData + 1);
	 
	if (RTP_DEPACKET_MAX_SIE - m_buffsize > nLength )
	{
		if (fua_head->s == 1)
		{//fua 第一包
			pH264Nalu |= (pData[0] & 0xE0); //获取 011  
			pH264Nalu |= (pData[1] & 0x1F); //获取 nalu 

			//拷贝视频起始头 00 00 00 01
			memcpy(m_buff + m_buffsize, SLICE_START_CODE, sizeof(SLICE_START_CODE));
			m_buffsize += sizeof(SLICE_START_CODE);

			//拷贝 Nalu 头
			memcpy(m_buff + m_buffsize, &pH264Nalu, 1);
			m_buffsize += 1;
		}
		//拷贝视频数据
		memcpy(m_buff + m_buffsize, pData + 2, nLength - 2);
		m_buffsize += nLength - 2;
	}
	else
	{//防止越界造成崩溃
		m_buffsize = 0;
		ret = e_rtpdepkt_err_bufferfull;
 	}

	return ret; 
}

//对h265进行解包 
int32_t depacket::handle_h265(unsigned char* pData, int nLength)
{
 	int32_t     ret = e_rtpdepkt_err_noerror;

#ifdef  H265_UseSplitterVideoFlag
	if (cbBuff == NULL)
		cbBuff = new unsigned char[RTP_DEPACKET_MAX_SIE];
#endif

	//如果时间戳不相同 并且 m_buffsize > 0 (m_buffsize 说明已经拼接过rtp包数据)
	if (::ntohl(rtpHead->timestamp) != timestamp && m_buffsize > 0)
	{
		cb.ssrc = ::ntohl(rtpHead->ssrc);
#ifdef  H265_UseSplitterVideoFlag
		//对h265视频包进行分割
 		SplitterRtpDepacketBuffer(m_buff, m_buffsize);
#else 
		cb.datasize = m_buffsize;
		cb.data = m_buff;
		m_cb(&cb);

		m_buffsize = 0;
#endif 
	}

	//获取h265的Nalu 
	nNalu = pData[0];
	nNalu = (nNalu & 0x7E) >> 1;
	
	if (nNalu < 48)
	{//单帧打包进行解包
		SingleDepacket(pData, nLength);
	}
	else if (nNalu == 48)
	{//多帧打包 STAP_A 模式进行解包，h265打包时Nalu头占2个字节，所以还需要rtp头后面往后偏移2字节
		Stap_aDepacket(pData +  2, nLength -  2);
	}
	else if (nNalu == 49)
	{//fus打包进行解包
		H265FusDepacket(pData, nLength);
	}
	else//不支持
		ret = e_rtpdepkt_err_unsupport;

	//更新时间戳
	timestamp = cb.timestamp = ::ntohl(rtpHead->timestamp);

	return ret;
}

//H265 fus 解包 
int32_t depacket::H265FusDepacket(unsigned char* pData, int nLength)
{
 	_rtp_fus_header* head = reinterpret_cast<_rtp_fus_header*>(pData + 2);
 
	if ((RTP_DEPACKET_MAX_SIE - m_buffsize) > (nLength - 3))
	{
		if (head->s) 
		{
			uint8_t nh[2] = { 0 };
			nh[0] = pData[0] & 0x81;
			nh[0] |= (pData[2] << 1) & 0x7e;
			nh[1] = pData[1];

			memcpy(m_buff + m_buffsize, RTP_H265_SLICESTARTCODE, sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t));
			m_buffsize += sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t);
			memcpy(m_buff + m_buffsize, nh, 2);
			m_buffsize += 2;
		}

		memcpy(m_buff + m_buffsize, pData+3, nLength - 3);
		m_buffsize += nLength - 3;

	    return e_rtpdepkt_err_noerror ;
	}
	else
	{
		m_buffsize = 0;
		return e_rtpdepkt_err_bufferfull;
	}
}