/*
功能：
       rtp打包功能实现文件，能实现对H264、H265、G711A、G711U、AAC 进行打包 

日期    2025-09-03
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "packet.h"
 
unsigned char pVideoStartCode_1[] = { 0x00,0x00,0x00,0x01 };
unsigned char pVideoStartCode_2[] = { 0x00,0x00,0x01 };

packet::packet()
{
	m_outbufsize = 0;
	m_seqbase = 0;
	m_ttbase = 0;
	m_out.data = m_outbuff;
}

packet::~packet()
{

}
 
bool packet::set_option(_rtp_packet_sessionopt* op)
{
	if (op)
	  memcpy((char*)&rtp_sessionOpt, (char*)op, sizeof(_rtp_packet_sessionopt));

	m_rtphead.ssrc = ::htonl(op->ssrc);
	m_rtphead.payload = op->payload;

	m_out.handle = nID;
	m_out.ssrc = op->ssrc;
	m_out.userdata = userdata;

	return true;
}
 
uint32_t packet::handle(uint8_t* data, uint32_t datasize, uint32_t inTimestamp)
{
	if (!data || (0 == datasize))
	{
		return e_rtppkt_err_invaliddata;
	}
	int32_t ret = e_rtppkt_err_noerror;
	m_inTimestamp = inTimestamp;

	switch (rtp_sessionOpt.streamtype)
	{
	    case e_rtppkt_st_h264:
			ret = SplitterH264VideoData(data, datasize);
		  break;
		case e_rtppkt_st_h265:
			ret = SplitterH265VideoData(data, datasize);
			break;
		case e_rtppkt_st_aac:
			aac_adts(data, datasize);
			break;
		default:
			{
			ret = handle_common(data, datasize);
			}
 	}

	if (m_inTimestamp == 0)
	{
		if (m_ttbase == 0xFFFFFFFF)
			m_ttbase = 0;//达到最大值进行翻转
		else 
		    m_ttbase += rtp_sessionOpt.ttincre;
 	}
	else
		m_ttbase = m_inTimestamp;

	return ret;
}

int32_t packet::handle_common(uint8_t* data, uint32_t datasize)
{
	int32_t ret = e_rtppkt_err_noerror;
	int nPos = 0, nSendLength = datasize;

	m_rtphead.ssrc      = htonl(rtp_sessionOpt.ssrc);
	m_rtphead.timestamp = htonl(m_ttbase);

	while (nSendLength > 0)
	{
		m_rtphead.seq = ::htons(m_seqbase);
		if (nSendLength > RTP_PAYLOAD_MAX_SIZE)
		{
	       m_rtphead.mark = 0 ;
		   memcpy(m_outbuff, (unsigned char*)&m_rtphead, sizeof(m_rtphead));
		   m_outbufsize = sizeof(m_rtphead);

		   memcpy(m_outbuff + m_outbufsize, data + nPos, RTP_PAYLOAD_MAX_SIZE);
		   m_outbufsize += RTP_PAYLOAD_MAX_SIZE;

		   nPos        += RTP_PAYLOAD_MAX_SIZE;
		   nSendLength -= RTP_PAYLOAD_MAX_SIZE;
		}
		else
		{
			m_rtphead.mark = 1;
			memcpy(m_outbuff, (unsigned char*)&m_rtphead, sizeof(m_rtphead));
			m_outbufsize = sizeof(m_rtphead);

			memcpy(m_outbuff + m_outbufsize, data + nPos, nSendLength);
			m_outbufsize += nSendLength ;
 
			nPos        = datasize;
			nSendLength = 0;
		}
  
		if (m_cb)
		{
			m_out.datasize = m_outbufsize;
			m_cb(&m_out);
		}

		if (m_seqbase == 0xFFFF)
			m_seqbase = 0;//达到最大值，进行翻转
		else 
 	        m_seqbase ++;
	}

	return ret;
}

//aac打包
int32_t packet::aac_adts(uint8_t* data, uint32_t datasize)
{
	if (datasize <= 7)
		return e_rtppkt_err_invalidparam;

	if ((data[0] == 0xFF && (data[1] & 0xF0) == 0xF0))
		AdtsHeaderLength = 7;//当有adts头时，去掉adts头
	else
		AdtsHeaderLength = 0;//没有时，不需要去掉 

	datasize -= AdtsHeaderLength;
	memset(aacBuffer, 0x00, sizeof(aacBuffer));
	aacBuffer[0] = 0x00;
	aacBuffer[1] = 0x10;
	aacBuffer[2] = (datasize & 0x1FE0) >> 5; //高8位
	aacBuffer[3] = (datasize & 0x1F) << 3; //低5位
	memcpy(aacBuffer + 4, data + AdtsHeaderLength, datasize);

	return handle_common(aacBuffer, datasize + 4);
}

//视频h264、h265单nalu 打包 
uint32_t packet::VideoSingleNalu(unsigned char* pFrameData, int nFrameLength, bool bLast)
{
	m_rtphead.payload = rtp_sessionOpt.payload;
	m_rtphead.ssrc = htonl(rtp_sessionOpt.ssrc);
	m_rtphead.seq = htons(m_seqbase);
	m_rtphead.mark = bLast == true ? 1 : 0;
	m_rtphead.timestamp = htonl(m_ttbase);

	if (m_cb)
	{//回调打包好的数据
		memcpy(m_outbuff,(char*)&m_rtphead, sizeof(m_rtphead));
		m_outbufsize = sizeof(m_rtphead);
		memcpy(m_outbuff + m_outbufsize, pFrameData, nFrameLength);
		m_outbufsize += nFrameLength;
		
		m_out.datasize = m_outbufsize;//指定输出长度
		m_cb(&m_out);
	}

	if (m_seqbase == 0xFFFF)
		m_seqbase = 0;//达到最大值，进行翻转
	else
		m_seqbase ++;

	return e_rtppkt_err_noerror;
}

//H264 Fua 打包  
uint32_t packet::H264FuaNalu(unsigned char* pFrameData, int nFrameLength, bool bLast)
{
	int nPos = 0;
	int nSendPacketLength = nFrameLength;
	unsigned char nNaluHead = pFrameData[0];
	_rtp_fua_indicator rtpFuaIndicator;
	_rtp_fua_header    fuaHeader;

	m_rtphead.payload = rtp_sessionOpt.payload;
	m_rtphead.ssrc = htonl(rtp_sessionOpt.ssrc);
	m_rtphead.timestamp = htonl(m_ttbase);

	rtpFuaIndicator.f = (nNaluHead >> 7) & 0x01;
	rtpFuaIndicator.n = (nNaluHead >> 5) & 0x03;

	fuaHeader.t = (nNaluHead & 0x1F);

	//使用了Nalu头的1个字节
	nPos += 1;
	nSendPacketLength -= 1;

	while (nSendPacketLength > 0)
	{
		m_rtphead.seq = htons(m_seqbase);
		if (nSendPacketLength > RTP_PAYLOAD_MAX_SIZE - 2)
		{//不是最后一段,以长度为 RTP_PAYLOAD_MAX_SIZE - 2 进行切割265视频数据, 因为 rtpFuaIndicator ， fuaHeader 占用了2个字节
 			m_rtphead.mark = 0;

			if (nPos == 1)
			{//第一个分包
				fuaHeader.s = 1;
				fuaHeader.e = 0;
			}
			else
			{
				fuaHeader.s = 0;
				fuaHeader.e = 0;
			}

		}
		else
		{//最后一段
 			m_rtphead.mark = 1;

			fuaHeader.s = 0;
			fuaHeader.e = 1;
		}

		if (m_cb)
		{//回调打包好的数据
			memcpy(m_outbuff, (char*)&m_rtphead, sizeof(m_rtphead));
			m_outbufsize = sizeof(m_rtphead);

			memcpy(m_outbuff + m_outbufsize,(char*)&rtpFuaIndicator, 1);
			m_outbufsize += 1;

			memcpy(m_outbuff + m_outbufsize, (char*)&fuaHeader, 1);
			m_outbufsize += 1;
			if (m_rtphead.mark == 0)
			{//中间包
				memcpy(m_outbuff + m_outbufsize, pFrameData + nPos, RTP_PAYLOAD_MAX_SIZE - 2);
				m_outbufsize += RTP_PAYLOAD_MAX_SIZE - 2;
			}
			else
			{//最后一包
				memcpy(m_outbuff + m_outbufsize, pFrameData + nPos, nFrameLength - nPos);
				m_outbufsize += nFrameLength - nPos;
			}

			m_out.datasize = m_outbufsize;//指定输出长度
			m_cb(&m_out);
		}

		if (m_seqbase == 0xFFFF)
			m_seqbase = 0;//达到最大值，进行翻转
		else
			m_seqbase++;

		if (m_rtphead.mark == 1)
		{//最后一包 
			nSendPacketLength = 0;
			nPos              = nFrameLength;
		}
		else
		{//中间包
			nSendPacketLength -= RTP_PAYLOAD_MAX_SIZE - 2;
			nPos              += RTP_PAYLOAD_MAX_SIZE - 2;
		}
	}

	return true;
}

//对H264进行切割并打包 
int32_t packet::SplitterH264VideoData(unsigned char* pVideoData, int nLength)
{
	int nPosStart = -1, nPosEnd = -1;
	int nStartCodeLength = 3;
	int nFrameLength = 0;
	
	if (!(memcmp(pVideoData, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0 || memcmp(pVideoData, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0))
		return e_rtppkt_err_invaliddata;//数据错误

	for (int i = 0; i < nLength; i++)
	{
		if (memcmp(pVideoData + i, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
		{
			if (nPosStart == -1 && nPosEnd == -1)
				nPosStart = i;
			else if (nPosStart >= 0)
				nPosEnd = i;

			i += sizeof(pVideoStartCode_1);
		}
		else if (memcmp(pVideoData + i, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
		{
			if (nPosStart == -1 && nPosEnd == -1)
				nPosStart = i;
			else if (nPosStart >= 0)
				nPosEnd = i;

			i += sizeof(pVideoStartCode_2);
		}
		else
		{
			continue;
		}

		//尚未找到一帧
		if (!(nPosStart >= 0 && nPosEnd >= 0))
			continue;

		//确定起始码的长度
		if (memcmp(pVideoData + nPosStart, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
			nStartCodeLength = 4;
		else if (memcmp(pVideoData + nPosStart, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
			nStartCodeLength = 3;
		else//错误的起始码
			return e_rtppkt_err_invaliddata;
		nFrameLength = nPosEnd - nPosStart - nStartCodeLength;

		if (nFrameLength > 0)
		{//确保数据合法
		   if (nFrameLength < RTP_PAYLOAD_MAX_SIZE)
			 VideoSingleNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, false);
		   else
			 H264FuaNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, false);
		}else
			return e_rtppkt_err_invaliddata;
		
		//交换nPosStart，进行查找下一帧 
		nPosStart = nPosEnd;
	}

    //确定起始码的长度
	if (memcmp(pVideoData + nPosStart, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
		nStartCodeLength = 4;
	else if (memcmp(pVideoData + nPosStart, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
		nStartCodeLength = 3;
	else//错误的起始码
		return e_rtppkt_err_invaliddata;

	nFrameLength = nLength - nPosStart - nStartCodeLength;

 	if (nFrameLength > 0)
	{//确保数据合法
		if (nFrameLength < RTP_PAYLOAD_MAX_SIZE)
			VideoSingleNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, true);
		else
			H264FuaNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, true);

		return e_rtppkt_err_noerror;
	}
	else
		return e_rtppkt_err_invaliddata;
}

//H265 Fus 打包  
uint32_t packet::H265FusNalu(unsigned char* pFrameData, int nFrameLength, bool bLast)
{
	int nPos = 0;
	int nSendPacketLength = nFrameLength;
	uint8_t            h265NaluHead[2];//265Nal头 
	unsigned char      nNaluHead = pFrameData[0];
	_rtp_fus_header    fusHeader;

	m_rtphead.payload = rtp_sessionOpt.payload;
	m_rtphead.ssrc = htonl(rtp_sessionOpt.ssrc);
	m_rtphead.timestamp = htonl(m_ttbase);

	h265NaluHead[0] = 49;
	h265NaluHead[0] = h265NaluHead[0] << 1;
	h265NaluHead[0] |= (pFrameData[0] & 0x80);
	h265NaluHead[0] |= (pFrameData[0] & 0x01);
	h265NaluHead[1]  = pFrameData[1];

	nNaluHead = ((pFrameData[0] & 0x7E) >> 1);
 	fusHeader.t = nNaluHead ;

	//使用了H265的Nalu头2个字节
	nPos += 2;
	nSendPacketLength -= 2;

	while (nSendPacketLength > 0)
	{
		m_rtphead.seq = htons(m_seqbase);
		if (nSendPacketLength > RTP_PAYLOAD_MAX_SIZE - 3)
		{//不是最后一段、以长度为 RTP_PAYLOAD_MAX_SIZE - 3 进行切割265视频数据 ，因为 h265NaluHead、fusHeader 总共占了3个字节
			m_rtphead.mark = 0;

			if (nPos == 2)
			{//第一个分包，nPos 为 2 ，不是264状态下的 1 
				fusHeader.s = 1;
				fusHeader.e = 0;
			}
			else
			{
				fusHeader.s = 0;
				fusHeader.e = 0;
			}

		}
		else
		{//最后一段
			m_rtphead.mark = 1;

			fusHeader.s = 0;
			fusHeader.e = 1;
		}

		if (m_cb)
		{//回调打包好的数据
			memcpy(m_outbuff, (char*)&m_rtphead, sizeof(m_rtphead));
			m_outbufsize = sizeof(m_rtphead);

			memcpy(m_outbuff + m_outbufsize, h265NaluHead, 2);
			m_outbufsize += 2;

			memcpy(m_outbuff + m_outbufsize, (char*)&fusHeader, 1);
			m_outbufsize += 1;
			if (m_rtphead.mark == 0)
			{//中间包
				memcpy(m_outbuff + m_outbufsize, pFrameData + nPos, RTP_PAYLOAD_MAX_SIZE - 3); //以长度为 RTP_PAYLOAD_MAX_SIZE - 3 进行切割265视频数据
				m_outbufsize += RTP_PAYLOAD_MAX_SIZE - 3;
			}
			else
			{//最后一包
				memcpy(m_outbuff + m_outbufsize, pFrameData + nPos, nFrameLength - nPos);
				m_outbufsize += nFrameLength - nPos;
			}

			m_out.datasize = m_outbufsize;//指定输出长度
			m_cb(&m_out);
		}

		if (m_seqbase == 0xFFFF)
			m_seqbase = 0;//达到最大值，进行翻转
		else
			m_seqbase++;

		if (m_rtphead.mark == 1)
		{//最后一包 
			nSendPacketLength = 0;
			nPos              = nFrameLength;
		}
		else
		{//中间包
			nSendPacketLength -= RTP_PAYLOAD_MAX_SIZE - 3; //以长度为 RTP_PAYLOAD_MAX_SIZE - 3 进行切割265视频数据
			nPos              += RTP_PAYLOAD_MAX_SIZE - 3; //以长度为 RTP_PAYLOAD_MAX_SIZE - 3 进行切割265视频数据
		}
	}

	return true;
}

//对H265进行切割并打包 
int32_t packet::SplitterH265VideoData(unsigned char* pVideoData, int nLength)
{
	int nPosStart = -1, nPosEnd = -1;
	int nStartCodeLength = 3;
	int nFrameLength = 0;
	
	if (!(memcmp(pVideoData, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0 || memcmp(pVideoData, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0))
		return e_rtppkt_err_invaliddata;//数据错误

	for (int i = 0; i < nLength; i++)
	{
		if (memcmp(pVideoData + i, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
		{
			if (nPosStart == -1 && nPosEnd == -1)
				nPosStart = i;
			else if (nPosStart >= 0)
				nPosEnd = i;

			i += sizeof(pVideoStartCode_1);
		}
		else if (memcmp(pVideoData + i, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
		{
			if (nPosStart == -1 && nPosEnd == -1)
				nPosStart = i;
			else if (nPosStart >= 0)
				nPosEnd = i;

			i += sizeof(pVideoStartCode_2);
		}
		else
		{
			continue;
		}

		//尚未找到一帧
		if (!(nPosStart >= 0 && nPosEnd >= 0))
			continue;

		//确定起始码的长度
		if (memcmp(pVideoData + nPosStart, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
			nStartCodeLength = 4;
		else if (memcmp(pVideoData + nPosStart, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
			nStartCodeLength = 3;
		else//错误的起始码
			return e_rtppkt_err_invaliddata;

		nFrameLength = nPosEnd - nPosStart - nStartCodeLength;

		if (nFrameLength > 0)
		{//确保数据合法
		   if (nFrameLength < RTP_PAYLOAD_MAX_SIZE)
			  VideoSingleNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, false);
		   else
			 H265FusNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, false);
		}else
			return e_rtppkt_err_invaliddata;
			
		//交换nPosStart，进行查找下一帧 
		nPosStart = nPosEnd;
	}

	//确定起始码的长度
	if (memcmp(pVideoData + nPosStart, pVideoStartCode_1, sizeof(pVideoStartCode_1)) == 0)
		nStartCodeLength = 4;
	else if (memcmp(pVideoData + nPosStart, pVideoStartCode_2, sizeof(pVideoStartCode_2)) == 0)
		nStartCodeLength = 3;
	else//错误的起始码
		return e_rtppkt_err_invaliddata;

	nFrameLength = nLength - nPosStart - nStartCodeLength;

	if (nFrameLength > 0)
	{//确保数据合法
		if (nFrameLength < RTP_PAYLOAD_MAX_SIZE)
			VideoSingleNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, true);
		else
			H265FusNalu(pVideoData + (nPosStart + nStartCodeLength), nFrameLength, true);

		return e_rtppkt_err_noerror;
	}
	else
		return e_rtppkt_err_invaliddata;
}
