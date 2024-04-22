#include "stdafx.h"
#include "RtcpPacket.h"
#include <time.h>
#ifndef OS_System_Windows
 #include <sys/time.h>
#endif

#define C1000000 1000000
#define CEPOCH   11644473600000000

const unsigned int CRtcpPacketBase::RTCP_MIN_RTP_LEN = 12;

CRtcpPacketBase::CRtcpPacketBase()
{
	m_rtcpHeader.version = 2;
	m_rtcpHeader.padding_flag = 0;
	m_rtcpHeader.report_count = 0;
	m_rtcpHeader.payload_type = 0;
	m_rtcpHeader.packet_length = 0;
}

CRtcpPacketBase::~CRtcpPacketBase()
{
   malloc_trim(0);
}

inline void CRtcpPacketBase::SetRtcpPacketType(unsigned char ucType)
{
	m_rtcpHeader.payload_type = ucType;
}

inline void CRtcpPacketBase::SetRtcpPacketLength(unsigned short usLength)
{
	m_rtcpHeader.packet_length = htons(usLength/4-1);
}

inline void CRtcpPacketBase::SetRtcpReceptionReportCount(unsigned char ucCount)
{
	m_rtcpHeader.report_count = (ucCount & 0x1f);
}

inline void CRtcpPacketBase::SetPaddingFlag(unsigned char ucFlag)
{
	m_rtcpHeader.padding_flag = (0 == ucFlag ? 0 : 1);
}

bool CRtcpPacketBase::GetSSRC(unsigned int& unSSRC)
{
	unSSRC = 0;
	return false;
}

CRtcpPacketSR::CRtcpPacketSR()
{
	SetRtcpPacketType(E_RTCP_PACKET_SR);
}

CRtcpPacketSR::~CRtcpPacketSR()
{
	for (std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.begin(); m_mapReportBlock.end() != it; ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}
	m_mapReportBlock.clear();
}

void CRtcpPacketSR::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{
	if (pRtpPacket && (unPacketSize >= RTCP_MIN_RTP_LEN))
	{
		if (0  != (pRtpPacket[0] & 0x80))	//检查version
		{
			unsigned int ssrc = ((pRtpPacket[8] << 24) | (pRtpPacket[9] << 16) | (pRtpPacket[10] << 8) | pRtpPacket[11]);
			CRtcpReportBlock* pReportBlock = NULL;

			/*
			std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.find(ssrc);
			if (m_mapReportBlock.end() != it)
			{
				pReportBlock = it->second;
			}
			else
			{
				pReportBlock = new(std::nothrow) CRtcpReportBlock(ssrc);
				if (pReportBlock)
				{
					m_mapReportBlock.insert(std::make_pair(ssrc, pReportBlock));	//新生成的report block对象存入容器
				}
			}

			if (pReportBlock)
			{
				pReportBlock->DealRtpPacket(pRtpPacket, unPacketSize);
			}
			*/
		}
	}
}

bool CRtcpPacketSR::BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc)
{
	unsigned int unNeedSize = 0;
	unNeedSize += sizeof(RTCP_HEADER);									//header
	unNeedSize += sizeof(unsigned int);									//ssrc
	unNeedSize += sizeof(RTCP_SR_SENDER_INFO);							//sender info
	unNeedSize += sizeof(RTCP_REPORT_BLOCK) * m_mapReportBlock.size();	//report block
	struct timeval tv;

	if (unPacketSize >= unNeedSize)
	{
		unsigned int offset = 0;
		unsigned int sender_ssrc = htonl(ssrc);
		unsigned int report_block_used_size = 0;
		SetRtcpPacketLength(unNeedSize);
		//设置sender info
		//......


		memcpy(pRtcpBuff + offset, &m_rtcpHeader, sizeof(m_rtcpHeader));	//header
		offset += sizeof(m_rtcpHeader);
		memcpy(pRtcpBuff + offset, &sender_ssrc, sizeof(sender_ssrc));		//ssrc
		offset += sizeof(sender_ssrc);
		memcpy(pRtcpBuff + offset, &m_senderInfo, sizeof(m_senderInfo));	//sender info
		offset += sizeof(m_senderInfo);
		for (std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.begin();
			m_mapReportBlock.end() != it; ++it)								//report block
		{
			if (it->second)
			{
				report_block_used_size = unPacketSize - offset;
				if (!it->second->BuildReportBlock(pRtcpBuff + offset, report_block_used_size))
				{
					unPacketSize = 0;
					return false;
				}

				offset += report_block_used_size;
			}
		}

		unPacketSize = unNeedSize;
		return true;
	}
	else
	{
		unPacketSize = 0;
		return false;
	}
}


CRtcpPacketRR::CRtcpPacketRR()
{
	SetRtcpPacketType(E_RTCP_PACKET_RR);
}

CRtcpPacketRR::~CRtcpPacketRR()
{
	for (std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.begin(); m_mapReportBlock.end() != it; ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}
	m_mapReportBlock.clear();
}

void CRtcpPacketRR::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{
	if (pRtpPacket && (unPacketSize >= RTCP_MIN_RTP_LEN))
	{
		if (0 != (pRtpPacket[0] & 0x80))	//检查version
		{
			unsigned int ssrc = ((pRtpPacket[8] << 24) | (pRtpPacket[9] << 16) | (pRtpPacket[10] << 8) | pRtpPacket[11]);
			CRtcpReportBlock* pReportBlock = NULL;

			std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.find(ssrc);
			if (m_mapReportBlock.end() != it)
			{
				pReportBlock = it->second;
			}
			else
			{
				pReportBlock = new(std::nothrow) CRtcpReportBlock(ssrc);
				if (pReportBlock)
				{
					m_mapReportBlock.insert(std::make_pair(ssrc, pReportBlock));	//新生成的report block对象存入容器
				}
			}

			if (pReportBlock)
			{
				pReportBlock->DealRtpPacket(pRtpPacket, unPacketSize);
			}
		}
	}
}

bool CRtcpPacketRR::BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc)
{
	unsigned int unNeedSize = 0;
	unNeedSize += sizeof(RTCP_HEADER);									//header
	unNeedSize += sizeof(unsigned int);									//ssrc
	unNeedSize += sizeof(RTCP_REPORT_BLOCK) * m_mapReportBlock.size();	//report block

	if (unPacketSize >= unNeedSize)
	{
		unsigned int offset = 0;
		unsigned int sender_ssrc = htonl(ssrc);
		unsigned int report_block_used_size = 0;
		SetRtcpPacketLength(unNeedSize);
		SetRtcpReceptionReportCount(m_mapReportBlock.size());
	
		memcpy(pRtcpBuff + offset, &m_rtcpHeader, sizeof(m_rtcpHeader));	//header
		offset += sizeof(m_rtcpHeader);
		memcpy(pRtcpBuff + offset, &sender_ssrc, sizeof(sender_ssrc));		//ssrc
		offset += sizeof(sender_ssrc);
		for (std::map<unsigned int, CRtcpReportBlock*>::iterator it = m_mapReportBlock.begin();
			m_mapReportBlock.end() != it; ++it)								//report block
		{
			if (it->second)
			{
				report_block_used_size = unPacketSize - offset;
				if (!it->second->BuildReportBlock(pRtcpBuff + offset, report_block_used_size))
				{
					unPacketSize = 0;
					return false;
				}

				offset += report_block_used_size;
			}
		}

		unPacketSize = unNeedSize;
		return true;
	}
	else
	{
		unPacketSize = 0;
		return false;
	}
}

bool CRtcpPacketRR::GetSSRC(unsigned int& unSSRC)
{
	if (m_mapReportBlock.size() > 0)
	{
		unSSRC = m_mapReportBlock.begin()->first;
		return true;
	}
	else
	{
		unSSRC = 0;
		return false;
	}
}


CRtcpPacketSDES::CRtcpPacketSDES()
{
	SetRtcpPacketType(E_RTCP_PACKET_SDES);
}

CRtcpPacketSDES::~CRtcpPacketSDES()
{

}

void CRtcpPacketSDES::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{

}

bool CRtcpPacketSDES::BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc)
{
	return false;
}

CRtcpPacketBYE::CRtcpPacketBYE()
{
	SetRtcpPacketType(E_RTCP_PACKET_BYE);
}

CRtcpPacketBYE::~CRtcpPacketBYE()
{

}

void CRtcpPacketBYE::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{

}

bool CRtcpPacketBYE::BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc)
{
	return false;
}

CRtcpPacketAPP::CRtcpPacketAPP()
{
	SetRtcpPacketType(E_RTCP_PACKET_APP);
}

CRtcpPacketAPP::~CRtcpPacketAPP()
{

}

void CRtcpPacketAPP::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{

}

bool CRtcpPacketAPP::BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc)
{
	return false;
}

const unsigned int CRtcpReportBlock::RTCP_MIN_RTP_LEN = 12;

CRtcpReportBlock::CRtcpReportBlock(unsigned int ssrc)
	: m_ssrc(ssrc)
	, m_bFirst(true)
	, m_nextSeq(0)
	, m_lossCount(0)
	, m_currentLossCount(0)
	, m_preSeqNum(0)
	, m_curSeqNum(0)
	, m_numcycles(0)
	, m_exthighseqnr(0)
	, m_bCycle(false)
	, m_preTimestamp(0)
	, m_curTimestamp(0)
	, m_preReceivedTime(0)
	, m_curRecvivedTime(0)
	, m_preTransit(0)
	, m_jitter(0)
	, m_lsr(0)
	, m_dlsr(0)
{
	m_reportBlock.ssrc_n = htonl(m_ssrc);
}

CRtcpReportBlock::~CRtcpReportBlock()
{

}

void CRtcpReportBlock::DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize)
{
	if (pRtpPacket && (unPacketSize > RTCP_MIN_RTP_LEN))
	{
		if (0 != (pRtpPacket[0] & 0x80))	//检查version
		{
			unsigned int ssrc = ((pRtpPacket[8] << 24) | (pRtpPacket[9] << 16) | (pRtpPacket[10] << 8) | pRtpPacket[11]);
			if (m_ssrc == ssrc)		//检查ssrc
			{
				unsigned short seq = ((pRtpPacket[2] << 8) | pRtpPacket[3]);
				unsigned int timestamp = ((pRtpPacket[4] << 24) | (pRtpPacket[5] << 16) | (pRtpPacket[6] << 8) | (pRtpPacket[7]));
				if (m_bFirst)
				{
					m_bFirst = false;
					m_nextSeq = seq + 1;
					m_preSeqNum = seq;
					m_curSeqNum = seq;
					/*
					m_preTimestamp = timestamp;
					m_preReceivedTime = GetReceivedTime();
					m_curTimestamp = timestamp;
					m_curTimestamp = m_preReceivedTime;
					m_exthighseqnr = seq;
					*/
				}
				else
				{
					if (m_nextSeq < seq )	//丢包
					{
						m_lossCount += (seq - m_nextSeq);
						m_currentLossCount += (seq - m_nextSeq);
						m_nextSeq = seq + 1;
						/*
						m_preTimestamp = timestamp;
						m_preReceivedTime = GetReceivedTime();
						m_curTimestamp = timestamp;
						m_curTimestamp = m_preReceivedTime;
						*/
	
						return;
					}
					else if (m_nextSeq > seq)		
					{
						if ((0xffff == m_nextSeq) && (0 == seq))   //兼容旧版xh流媒体不使用值为0xffff的sequence number的情况
						{
							m_nextSeq = seq + 1;
						}
						else if (  (m_nextSeq - seq) >= 0xfff0 )	//翻转时丢包
						{
							m_lossCount += (seq - m_nextSeq);
							m_currentLossCount += (seq - m_nextSeq);
							m_nextSeq = seq + 1;
							/*
							m_preTimestamp = timestamp;
							m_preReceivedTime = GetReceivedTime();
							m_curTimestamp = timestamp;
							m_curTimestamp = m_preReceivedTime;
							*/

							return;
						}
						else
						{
							return;
						}
					}

					m_curSeqNum = seq;
					/*
					m_curTimestamp = timestamp;
					m_curRecvivedTime = GetReceivedTime();
					*/

					//extend highest sequence number received
					if (seq >= (m_exthighseqnr & 0x0000ffff))
					{
						m_bCycle = true;
					}
					else
					{
						if (m_bCycle)
						{
							m_numcycles += 0x00010000;
							m_bCycle = false;
						}
					}

					m_exthighseqnr = m_numcycles + seq;

					//interarrival jitter
					/*
					double transit = m_curRecvivedTime - m_curTimestamp;
					double d = transit - m_preTransit;
					m_preTransit = transit;
					if (d < 0)
					{
						d = -d;
					}
					m_jitter += (1. / 16.) * ((double)d - m_jitter);
					*/
				}
			}
		}
	}
}

bool CRtcpReportBlock::BuildReportBlock(unsigned char* pRtcpBuff, unsigned int& unSize)
{
	if (pRtcpBuff && (unSize >= sizeof(RTCP_REPORT_BLOCK)))
	{
		m_reportBlock.fraction_lost = (m_curSeqNum == m_preSeqNum) ? 0 : (uint8_t)((double)m_currentLossCount / (m_curSeqNum - m_preSeqNum)*256.0);
		m_reportBlock.cumulative_pakcet_lost[2] = (uint8_t)(m_lossCount & 0xFF);
		m_reportBlock.cumulative_pakcet_lost[1] = (uint8_t)((m_lossCount >> 8) & 0xFF);
		m_reportBlock.cumulative_pakcet_lost[0] = (uint8_t)((m_lossCount >> 16) & 0xFF);
		m_reportBlock.extended_highest_seq = htonl(m_exthighseqnr);
		m_reportBlock.interarrival_jitter = htonl(m_jitter);
		m_reportBlock.LSR = htonl(m_lsr);
		m_reportBlock.DLSR = htonl(m_dlsr);

		memcpy(pRtcpBuff, &m_reportBlock, sizeof(RTCP_REPORT_BLOCK));
		unSize = sizeof(RTCP_REPORT_BLOCK);
		OnBuildReportBlock();
		return true;
	}
	else
	{
		unSize = 0;
		return false;
	}
}


void CRtcpReportBlock::SetLSR(unsigned int lsr)
{
	m_lsr = lsr;
}

void CRtcpReportBlock::SetDLSR(unsigned int dlsr)
{
	m_dlsr = dlsr;
}

void CRtcpReportBlock::OnBuildReportBlock()
{
	m_currentLossCount = 0;
	m_preSeqNum = m_curSeqNum;
	/*
	m_preTimestamp = m_curTimestamp;
	m_preReceivedTime = m_curRecvivedTime;
	*/
}

/*
double CRtcpReportBlock::GetReceivedTime()
{
// 	struct timeval tv;
// 	gettimeofday(&tv, 0);
// 
// 	double mt;
// 	if (tv.tv_sec >= 0)
// 	{
// 		mt = (double)tv.tv_sec + 1e-6*(double)tv.tv_usec;
// 	}
// 	else
// 	{
// 		int64_t possec = -tv.tv_sec;
// 
// 		mt = (double)possec + 1e-6*(double)tv.tv_usec;
// 		mt = -mt;
// 	}
// 
// 	return mt;
	static int inited = 0;
	static unsigned long long microseconds, initmicroseconds;
	static LARGE_INTEGER performancefrequency;

	unsigned long long emulate_microseconds, microdiff;
	SYSTEMTIME systemtime;
	FILETIME filetime;

	LARGE_INTEGER performancecount;

	QueryPerformanceCounter(&performancecount);

	if (!inited)
	{
		inited = 1;
		QueryPerformanceFrequency(&performancefrequency);
		GetSystemTime(&systemtime);
		SystemTimeToFileTime(&systemtime, &filetime);
		microseconds = (((unsigned long long)(filetime.dwHighDateTime) << 32) + (unsigned long long)(filetime.dwLowDateTime)) / (unsigned long long)10;
		microseconds -= CEPOCH; // EPOCH
		initmicroseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);
	}

	emulate_microseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);

	microdiff = emulate_microseconds - initmicroseconds;

	double t = 1e-6*(double)(microseconds + microdiff);
	return t;
}
*/

inline unsigned long long CRtcpReportBlock::CalculateMicroseconds(unsigned long long performancecount, unsigned long long performancefrequency)
{
	unsigned long long f = performancefrequency;
	unsigned long long a = performancecount;
	unsigned long long b = a / f;
	unsigned long long c = a%f; // a = b*f+c => (a*1000000)/f = b*1000000+(c*1000000)/f

	return b*C1000000 + (c*C1000000) / f;
}