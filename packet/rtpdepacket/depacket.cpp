#if (defined _WIN32 || defined _WIN64)
#include <WinSock2.h>
#pragma  comment(lib, "WS2_32.lib")
#else
#include <arpa/inet.h>
#endif
#include <sstream>
#include <malloc.h>
#include <cstring>
#include "depacket.h"
#include "rtp_def.h"

  
const uint8_t NALU_START_CODE[] = { 0x00, 0x00, 0x01 };
const uint8_t SLICE_START_CODE[] = { 0X00, 0X00, 0X00, 0X01 };

rtp_depacket::rtp_depacket(uint32_t ssrc, rtp_depacket_callback cb, void* userdata, uint32_t h)
	: m_ssrc(ssrc)
	, m_cb(cb)
	, m_userdata(userdata)
	, m_handle(h)
	, m_bufsize(0)
	, m_firstpkt(true)
	, m_inlost(false)
	, m_nextseq(0)
	, m_lasttimestamp(0)
	, m_lostpkt(0)
{
	memset(&m_out, 0, sizeof(_rtp_depacket_cb));

	m_out.handle = m_handle;
	m_out.ssrc = m_ssrc;
	m_out.userdata = const_cast<void*>(m_userdata);
}

rtp_depacket::~rtp_depacket()
{
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

void rtp_depacket::set_payload(uint8_t payload, uint32_t streamtype)
{
	m_payloadMap[payload] = streamtype;
}

uint32_t rtp_depacket::get_payload(uint8_t payload)
{
	uint32_t streamtype = e_rtpdepkt_st_unknow;

	auto it = m_payloadMap.find(payload);
	if (m_payloadMap.end() != it)
	{
		streamtype = it->second;
	}

	return streamtype;
}

void rtp_depacket::set_mediaoption(const std::string& opt, const std::string& param)
{
	if (!opt.empty())
	{
		m_mediaoptMap[opt] = param;
	}
}

template<typename T> bool rtp_depacket::get_mediaoption(const std::string& opt, T& val)
{
	auto it = m_mediaoptMap.find(opt);
	if (m_mediaoptMap.end() == it)
	{
		return false;
	}
	std::stringstream sstr;
	sstr << it->second;
	sstr >> val;
	//try
	//{
	//	
	//	val = boost::lexical_cast<T>(it->second.c_str());
	//}
	//catch (const boost::bad_lexical_cast& /*e*/)
	//{
	//	return false;
	//}
	//catch (...)
	//{
	//	return false;
	//}

	return true;
}

int32_t rtp_depacket::handle(uint8_t* data, uint32_t datasize)
{
	_rtp_header* head = reinterpret_cast<_rtp_header*>(data);

    m_out.ssrc = head->ssrc;
	
	//lost check
	if (m_inlost)
	{
		if (m_lasttimestamp != static_cast<uint32_t>(::ntohl(head->timestamp)))
		{
			m_lasttimestamp = static_cast<uint32_t>(::ntohl(head->timestamp));
			m_nextseq = static_cast<uint16_t>(::ntohs(head->seq));
			m_inlost = false;
			m_bufsize = 0;
			m_firstpkt = true;
		}
		else
		{
			++m_lostpkt;
			return e_rtpdepkt_err_inlost;
		}
	}

	//seq check
	if (m_firstpkt)
	{
		m_firstpkt = false;
	}
	else
	{
		if (m_nextseq > static_cast<uint16_t>(::ntohs(head->seq)))
		{
			if (!is_seq_flip(m_nextseq, static_cast<uint16_t>(::ntohs(head->seq))))
			{
				m_inlost = true;

				++m_lostpkt;
				return e_rtpdepkt_err_seqerror;
			}
		}
		else if (m_nextseq < static_cast<uint16_t>(::ntohs(head->seq)))
		{
			++m_lostpkt;
			m_inlost = true;
			return e_rtpdepkt_err_seqerror;
		}
		else
		{
			;
		}
	}

	//callback
	if ((m_lasttimestamp != static_cast<uint32_t>(::ntohl(head->timestamp))) && (m_bufsize > 0))
	{
		m_out.timestamp = m_lasttimestamp;
		m_out.data = m_buff;
		m_out.datasize = m_bufsize;
		m_out.streamtype = get_payload(m_out.payload);

		if (m_cb)
		{
			m_cb(&m_out);
		}

		m_bufsize = 0;
	}

	//handle rtp packet
	m_out.payload = head->payload;

	uint32_t cclen = 0, ehlen = 0, paddinglen = 0;

	if (head->cc)
	{
		cclen = 4 * head->cc;
	}

	if (head->x)
	{
		ehlen = 4 + ((data[sizeof(_rtp_header) + cclen + 2] << 8) | data[sizeof(_rtp_header) + cclen + 3]) * 4;
	}

	if (head->p)
	{
		paddinglen = static_cast<uint32_t>(data[datasize - 1]);
	}

	if (datasize > (sizeof(_rtp_header) + cclen + ehlen + paddinglen))
	{
		uint8_t* addr = data + sizeof(_rtp_header) + cclen + ehlen;
		uint32_t  len = datasize - (sizeof(_rtp_header) + cclen + ehlen + paddinglen);

		if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + len))
		{
			++m_lostpkt;
			m_inlost = true;

			return e_rtpdepkt_err_bufferfull;
		}
		else
		{
			switch (get_payload(head->payload))
			{
			case e_rtpdepkt_st_h264:
				handle_h264(addr, len);
				break;
			case e_rtpdepkt_st_h265:
				handle_h265(addr, len);
				break;
			case e_rtpdepkt_st_aac:
				handle_common(addr, len);
				break;
			default:
				handle_common(addr, len);
				break;
			}
		}
	}

	//update timestamp and next seq
	m_lasttimestamp = static_cast<uint32_t>(::ntohl(head->timestamp));
	m_nextseq = static_cast<uint16_t>(::ntohs(head->seq)) + 1;

	return e_rtpdepkt_err_noerror;
}

bool rtp_depacket::is_seq_flip(uint16_t except, uint16_t actual)
{
	return ((except > actual) && ((except - actual) > 60000));
}

void rtp_depacket::handle_h264(uint8_t* data, uint32_t datasize)
{
	uint8_t modetype = data[0] & 0x1f;

	switch (modetype)
	{
	case e_rtp_h264mode_stapa:
		h264_stapa(data, datasize);
		break;
	case e_rtp_h264mode_fua:
		h264_fua(data, datasize);
		break;
	default:
	{
		if ((modetype >= e_rtp_h264mode_minnalunit) && (modetype <= e_rtp_h264mode_maxnalunit))
		{
			h264_nalunit(data, datasize);
		}
	}
	break;
	}
}

void rtp_depacket::h264_nalunit(uint8_t* data, uint32_t datasize)
{
	if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + datasize + sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t)))
	{
		++m_lostpkt;
		m_inlost = true;

		return;
	}

	memcpy(m_buff + m_bufsize, RTP_H264_SLICESTARTCODE, sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t));
	m_bufsize += sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t);
	memcpy(m_buff + m_bufsize, data, datasize);
	m_bufsize += datasize;
}

void rtp_depacket::h264_stapa(uint8_t* data, uint32_t datasize)
{
	uint16_t nalulen = 0;
	uint8_t* addr = data + 1;
	uint32_t len = datasize - 1;

	while (len > 2)
	{
		//nalu size
		memcpy(&nalulen, addr, 2);
		addr += 2;
		len -= 2;

		//nalu unit
		nalulen = static_cast<uint16_t>(::ntohs(nalulen));
		if ((len < nalulen) || (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + nalulen + sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t))))
		{
			++m_lostpkt;
			m_inlost = true;

			return;
		}

		if (nalulen > 0)
		{
			memcpy(m_buff + m_bufsize, RTP_H264_SLICESTARTCODE, sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t));
			m_bufsize += sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t);
			memcpy(m_buff + m_bufsize, addr, nalulen);
			m_bufsize += nalulen;

			addr += nalulen;
			len -= nalulen;
		}
	}
}

void rtp_depacket::h264_fua(uint8_t* data, uint32_t datasize)
{
	if (datasize <= 2)
	{
		return;
	}

	_rtp_fua_indicator* indct = reinterpret_cast<_rtp_fua_indicator*>(data);
	_rtp_fua_header* head = reinterpret_cast<_rtp_fua_header*>(data + sizeof(_rtp_fua_indicator));

	uint8_t* addr = data + 2;
	uint32_t len = datasize - 2;
	uint32_t alllen = (head->s ? sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t) + 1 : 0) + len;

	if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + alllen))
	{
		++m_lostpkt;
		m_inlost = true;

		return;
	}

	if (head->s) //first fu-a segment
	{
		uint8_t nalutype = 0;
		nalutype |= (data[0] & 0xe0);
		nalutype |= (data[1] & 0x1f);

		memcpy(m_buff + m_bufsize, RTP_H264_SLICESTARTCODE, sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t));
		m_bufsize += sizeof(RTP_H264_SLICESTARTCODE) / sizeof(uint8_t);
		memcpy(m_buff + m_bufsize, &nalutype, 1);
		++m_bufsize;
	}

	memcpy(m_buff + m_bufsize, addr, len);
	m_bufsize += len;
}

void rtp_depacket::handle_h265(uint8_t* data, uint32_t datasize)
{
	uint8_t modetype = (data[0] >> 1) & 0x3f;

	switch (modetype)
	{
	case e_rtp_h265mode_ap:
		h265_ap(data, datasize);
		break;
	case e_rtp_h265mode_fu:
		h265_fu(data, datasize);
		break;
	case e_rtp_h265mode_paci:
		h265_paci(data, datasize);
	default:
	{
		if ((modetype >= e_rtp_h265mode_minnalunit) && (modetype <= e_rtp_h265mode_maxnalunit))
		{
			h265_nalunit(data, datasize);
		}
		else if (modetype <= e_rtp_h265mode_maxnallike)
		{
			h265_nallike(data, datasize);
		}
		else
		{
			;
		}
	}
	break;
	}
}

void rtp_depacket::h265_nalunit(uint8_t* data, uint32_t datasize)
{
	uint32_t minlen = 2;
	uint32_t alllen = sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t) + datasize;
	int32_t smdd = 0;
	if (get_mediaoption("sprop-max-don-diff", smdd) && (0 != smdd))
	{
		minlen += 2;
		alllen -= 2;
	}

	if (datasize < minlen)
	{
		return;
	}

	if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + alllen))
	{
		++m_lostpkt;
		m_inlost = true;

		return;
	}

	memcpy(m_buff + m_bufsize, RTP_H265_SLICESTARTCODE, sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t));
	m_bufsize += sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t);
	memcpy(m_buff + m_bufsize, data, 2);
	m_bufsize += 2;
	memcpy(m_buff + m_bufsize, data + 2 + (0 != smdd ? 2 : 0), datasize - 2 - (0 != smdd ? 2 : 0));
	m_bufsize += (datasize - 2 - (0 != smdd ? 2 : 0));
}

void rtp_depacket::h265_ap(uint8_t* data, uint32_t datasize)
{
	if (datasize < 4)
	{
		return;
	}

	uint16_t nalulen = 0;
	uint8_t* addr = data + 2;
	uint32_t len = datasize - 2;
	uint32_t minlen = 2;
	int32_t smdd = 0;
	bool firstnal = true;

	if (get_mediaoption("sprop-max-don-diff", smdd) && (0 != smdd))
	{
		minlen += 1;
	}

	while (len > minlen)
	{
		//donl/dond
		if (0 != smdd)
		{
			addr += (firstnal ? 2 : 1);
			len -= (firstnal ? 2 : 1);
		}

		//nalu size
		memcpy(&nalulen, addr, 2);
		addr += 2;
		len -= 2;

		//nalu unit
		nalulen = static_cast<uint16_t>(::ntohs(nalulen));
		if ((len < nalulen) || (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + nalulen + sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t))))
		{
			++m_lostpkt;
			m_inlost = true;

			return;
		}

		if (nalulen > 0)
		{
			memcpy(m_buff + m_bufsize, RTP_H265_SLICESTARTCODE, sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t));
			m_bufsize += sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t);
			memcpy(m_buff + m_bufsize, addr, nalulen);
			m_bufsize += nalulen;

			addr += nalulen;
			len -= nalulen;
		}

		//first nalu flag
		firstnal = false;
	}
}

void rtp_depacket::h265_fu(uint8_t* data, uint32_t datasize)
{
	uint32_t minlen = 3;
	int32_t smdd = 0;

	if (get_mediaoption("sprop-max-don-diff", smdd) && (0 != smdd))
	{
		minlen += 2;
	}

	if (datasize < minlen)
	{
		return;
	}

	_rtp_fus_header* head = reinterpret_cast<_rtp_fus_header*>(data + 2);
	uint8_t* addr = data + 3 + (0 != smdd ? 2 : 0);
	uint32_t len = datasize - 3 - (0 != smdd ? 2 : 0);
	uint32_t alllen = (head->s ? sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t) + 2 : 0) + len;

	if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + alllen))
	{
		++m_lostpkt;
		m_inlost = true;

		return;
	}

	if (head->s) //first fu-a segment
	{
		uint8_t nh[2] = { 0 };
		nh[0] = data[0] & 0x81;
		nh[0] |= (data[2] << 1) & 0x7e;
		nh[1] = data[1];

		memcpy(m_buff + m_bufsize, RTP_H265_SLICESTARTCODE, sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t));
		m_bufsize += sizeof(RTP_H265_SLICESTARTCODE) / sizeof(uint8_t);
		memcpy(m_buff + m_bufsize, nh, 2);
		m_bufsize += 2;
	}

	memcpy(m_buff + m_bufsize, addr, len);
	m_bufsize += len;

}

void rtp_depacket::h265_paci(uint8_t* data, uint32_t datasize)
{
	if (datasize < 4)
	{
		return;
	}

	_rtp_paci_header* head = reinterpret_cast<_rtp_paci_header*>(data);
	uint32_t minlen = 4 + head->phs;
	uint8_t* addr = data + 4 + head->phs;
	uint32_t len = datasize - 4 - head->phs;
	uint32_t alllen = 2 + len;
	
	if (datasize < minlen)
	{
		return;
	}

	if (DEPACKET_DATA_BUFFER_MAX_SIZE < (m_bufsize + alllen))
	{
		++m_lostpkt;
		m_inlost = true;

		return;
	}

	uint16_t nalutype = 0;
	nalutype |= ((static_cast<uint16_t>(head->a)) << 15);
	nalutype |= ((static_cast<uint16_t>(data[0] & 0x01)) << 8);
	nalutype |= data[1];
	nalutype |= ((static_cast<uint16_t>(head->c)) << 9);

	memcpy(m_buff + m_bufsize, &nalutype, 2);
	m_bufsize += 2;
	memcpy(m_buff + m_bufsize, addr, len);
	m_bufsize += len;
}

void rtp_depacket::h265_nallike(uint8_t* data, uint32_t datasize)
{
	h265_paci(data, datasize);
}

void rtp_depacket::handle_common(uint8_t* data, uint32_t datasize)
{
	memcpy(m_buff + m_bufsize, data, datasize);
	m_bufsize += datasize;
}

//AAC解包，需要去掉前面4个字节的头
void rtp_depacket::handle_aac(uint8_t* data, uint32_t datasize)
{
	memcpy(m_buff + m_bufsize, data + 4, datasize - 4);
	m_bufsize += datasize - 4;
}

