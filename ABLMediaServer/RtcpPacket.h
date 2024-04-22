#pragma once

#include <map>
#include "RtcpDef.h"

class CRtcpReportBlock;

class CRtcpPacketBase
{
public:
	CRtcpPacketBase();
	~CRtcpPacketBase();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize) = 0;
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc) = 0;

	virtual void SetRtcpPacketType(unsigned char ucType);
	virtual void SetRtcpPacketLength(unsigned short usLength);
	virtual void SetRtcpReceptionReportCount(unsigned char ucCount);
	virtual void SetPaddingFlag(unsigned char ucFlag);
	virtual bool GetSSRC(unsigned int& unSSRC);

	enum E_RTCP_PACKET_TYPE
	{
		E_RTCP_PACKET_SR = 200,
		E_RTCP_PACKET_RR,
		E_RTCP_PACKET_SDES,
		E_RTCP_PACKET_BYE,
		E_RTCP_PACKET_APP
	};

protected:
	CRtcpPacketBase(const CRtcpPacketBase& obj);
	CRtcpPacketBase& operator=(const CRtcpPacketBase& obj);

protected:
	RTCP_HEADER m_rtcpHeader;
	static const unsigned int RTCP_MIN_RTP_LEN;
}; 

class CRtcpPacketSR : public CRtcpPacketBase
{
public:
	CRtcpPacketSR();
	~CRtcpPacketSR();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize) ;
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc);

private:
	RTCP_SR_SENDER_INFO m_senderInfo;
	std::map<unsigned int, CRtcpReportBlock*> m_mapReportBlock;
};

class CRtcpPacketRR : public CRtcpPacketBase
{
public:
	CRtcpPacketRR();
	~CRtcpPacketRR();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize);
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc);
	virtual bool GetSSRC(unsigned int& unSSRC);

private:
	std::map<unsigned int, CRtcpReportBlock*> m_mapReportBlock;
};

class CRtcpPacketSDES : public CRtcpPacketBase
{
public:
	CRtcpPacketSDES();
	~CRtcpPacketSDES();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize);
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc);
};

class CRtcpPacketBYE : public CRtcpPacketBase
{
public:
	CRtcpPacketBYE();
	~CRtcpPacketBYE();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize);
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc);
};

class CRtcpPacketAPP : public CRtcpPacketBase
{
public:
	CRtcpPacketAPP();
	~CRtcpPacketAPP();

	virtual void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize);
	virtual bool BuildRtcpPacket(unsigned char* pRtcpBuff, unsigned int& unPacketSize, unsigned int ssrc);

};

class CRtcpReportBlock
{
public:
	CRtcpReportBlock(unsigned int ssrc);
	~CRtcpReportBlock();

	void DealRtpPacket(const unsigned char* pRtpPacket, unsigned int unPacketSize);
	bool BuildReportBlock(unsigned char* pRtcpBuff, unsigned int& unSize);

	void SetLSR(unsigned int lsr);
	void SetDLSR(unsigned int dlsr);

private:
	CRtcpReportBlock(const CRtcpReportBlock& obj);
	CRtcpReportBlock& operator=(const CRtcpReportBlock& obj);

	void OnBuildReportBlock();
	//double GetReceivedTime();
	unsigned long long CalculateMicroseconds(unsigned long long performancecount, unsigned long long performancefrequency);

private:
	RTCP_REPORT_BLOCK m_reportBlock;

	unsigned int m_ssrc;
	bool m_bFirst;
	static const unsigned int RTCP_MIN_RTP_LEN;

	//fraction_lost cumulative_pakcet_lost 
	unsigned short m_nextSeq;
	unsigned int m_lossCount;
	unsigned int m_currentLossCount;
	unsigned short m_preSeqNum;
	unsigned short m_curSeqNum;

	//extended_highest_seq
	unsigned int m_numcycles;
	unsigned int m_exthighseqnr;
	bool m_bCycle;

	//interarrival_jitter
	unsigned int m_preTimestamp;
	unsigned int m_curTimestamp;
	double m_preReceivedTime;
	double m_curRecvivedTime;
	double m_preTransit;
	
	unsigned int m_jitter;

	//LSR
	unsigned int m_lsr;

	//DLSR
	unsigned int m_dlsr;
};