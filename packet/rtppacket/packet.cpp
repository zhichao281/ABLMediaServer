#if (defined _WIN32 || defined _WIN64)
#include <WinSock2.h>
#pragma  comment(lib, "WS2_32.lib")
#else
#include <arpa/inet.h>
#endif

#include "packet.h"
#include <memory.h>
#include <malloc.h>
 
const uint8_t NALU_START_CODE_RTPPACKET[] = { 0x00, 0x00, 0x01 };
const uint8_t SLICE_START_CODE_RTPPACKET[] = { 0X00, 0X00, 0X00, 0X01 };

rtp_packet::rtp_packet(rtp_packet_callback cb, void* userdata, const _rtp_packet_sessionopt& opt)
	: m_cb(cb)
	, m_userdata(userdata)
	, m_attr(opt)
	, RTP_PAYLOAD_MAX_SIZE(1320)
	, m_outbufsize(0)
	, m_ttbase(0)
	, m_seqbase(0)
{
	m_out.handle = opt.handle;
	m_out.ssrc = m_attr.ssrc;
	m_out.userdata = userdata;

	m_rtphead.payload = m_attr.payload;
	m_rtphead.ssrc = ::htonl(m_attr.ssrc);
}

rtp_packet::~rtp_packet()
{
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

int32_t rtp_packet::handle(uint8_t* data, uint32_t datasize, uint32_t inTimestamp)
{
	if (!data || (0 == datasize))
	{
		return e_rtppkt_err_invaliddata;
	}
	m_inTimestamp = inTimestamp;
	int32_t ret = e_rtppkt_err_noerror;

	switch (m_attr.streamtype)
	{
	case e_rtppkt_st_h264:
	case e_rtppkt_st_h265:
	case e_rtppkt_st_svacv:
	{
		ret = handle_nalu_stream(m_attr.streamtype, data, datasize);

	}break;

	case e_rtppkt_st_aac:
		ret = aac_adts(data, datasize);
		break;

	default:
	{
		ret = handle_common(data, datasize);

	};

	}

	
	return ret;
}

int32_t rtp_packet::handle_nalu_stream(int32_t st, uint8_t* data, uint32_t datasize)
{
	if ((datasize < sizeof(SLICE_START_CODE_RTPPACKET + 2)) ||
		((0 != memcmp(SLICE_START_CODE_RTPPACKET, data, sizeof(SLICE_START_CODE_RTPPACKET))) &&
		(0 != memcmp(NALU_START_CODE_RTPPACKET, data, sizeof(NALU_START_CODE_RTPPACKET)))))
	{
		return e_rtppkt_err_malformedframe;
	}

	int32_t ret = e_rtppkt_err_noerror;
	uint32_t payloadlen = 0;
	uint32_t sclen = 0, firstsclen = 0;
	bool findfirst = false;
	uint32_t pos = 0;

	for (uint32_t offset = 0; offset < (datasize - sizeof(SLICE_START_CODE_RTPPACKET));)
	{
		if (0 == memcmp(SLICE_START_CODE_RTPPACKET, data + offset, sizeof(SLICE_START_CODE_RTPPACKET)))
		{
			sclen = sizeof(SLICE_START_CODE_RTPPACKET);
		}
		else if (0 == memcmp(NALU_START_CODE_RTPPACKET, data + offset, sizeof(NALU_START_CODE_RTPPACKET)))
		{
			sclen = sizeof(NALU_START_CODE_RTPPACKET);
		}
		else
		{
			++offset;
			continue;
		}

		if (!findfirst)
		{
			findfirst = true;
			pos = offset;
			firstsclen = sclen;

			offset += (sclen + 1);

			continue;
		}

		payloadlen = offset - pos - firstsclen;

		if (payloadlen <= RTP_PAYLOAD_MAX_SIZE)
		{
			ret = singlenalu(data + pos + firstsclen, payloadlen, false);
		}
		else
		{
			switch (st)
			{
			case e_rtppkt_st_h264:
			{
				ret = h264_fua(data + pos + firstsclen, payloadlen, false);

			}break;

			case e_rtppkt_st_h265:
			{
				ret = h265_fus(data + pos + firstsclen, payloadlen, false);

			}break;

			case e_rtppkt_st_svacv:
			{
				ret = svacv_fua(data + pos + firstsclen, payloadlen, false);

			}break;

			default:
			{
				return e_rtppkt_err_unsupportednalustream;

			}break;

			}

		}

		if (e_rtppkt_err_noerror != ret)
		{
			if (m_inTimestamp == 0)
				m_ttbase += m_attr.ttincre;
			else
				m_ttbase = m_inTimestamp;

			return ret;
		}

		pos = offset;
		firstsclen = sclen;
		offset += (sclen + 1);
	}

	if (findfirst)
	{
		payloadlen = datasize - pos - firstsclen;

		if (payloadlen <= RTP_PAYLOAD_MAX_SIZE)
		{
			ret = singlenalu(data + pos + firstsclen, payloadlen, true);
		}
		else
		{
			switch (st)
			{
			case e_rtppkt_st_h264:
			{
				ret = h264_fua(data + pos + firstsclen, payloadlen, true);

			}break;

			case e_rtppkt_st_h265:
			{
				ret = h265_fus(data + pos + firstsclen, payloadlen, true);

			}break;

			case e_rtppkt_st_svacv:
			{
				ret = svacv_fua(data + pos + firstsclen, payloadlen, true);

			}break;

			default:
			{
				return e_rtppkt_err_unsupportednalustream;

			}break;

			}
		}
	}

	if (m_inTimestamp == 0)
		m_ttbase += m_attr.ttincre;
	else
		m_ttbase = m_inTimestamp;


	return ret;
}

int32_t rtp_packet::singlenalu(uint8_t* data, uint32_t datasize, bool last)
{
	uint8_t paddinglen = 0;

	m_outbufsize = 0;

	if (e_rtppkt_am_4octet == m_attr.alimod)
	{
		paddinglen = (0 == (sizeof(m_rtphead) + datasize) % 4) ? 0 : (4 - (sizeof(m_rtphead) + datasize) % 4);
	}
	else if (e_rtppkt_am_8octet == m_attr.alimod)
	{
		paddinglen = (0 == (sizeof(m_rtphead) + datasize) % 8) ? 0 : (8 - (sizeof(m_rtphead) + datasize) % 8);
	}
	else
	{
		paddinglen = 0;
	}

	m_rtphead.p = (0 == paddinglen) ? 0 : 1;
	m_rtphead.mark = (last ? 1 : 0);
	m_rtphead.seq = ::htons(m_seqbase);
	m_rtphead.timestamp = ::htonl(m_ttbase);


	memcpy(m_outbuff + m_outbufsize, &m_rtphead, sizeof(m_rtphead));
	m_outbufsize += sizeof(m_rtphead);

	memcpy(m_outbuff + m_outbufsize, data, datasize);
	m_outbufsize += datasize;

	if (0 != paddinglen)
	{
		if (paddinglen - 1 > 0)
		{
			memset(m_outbuff + m_outbufsize, 0x00, paddinglen - 1);
			m_outbufsize += (paddinglen - 1);
		}

		memcpy(m_outbuff + m_outbufsize, &paddinglen, 1);
		++m_outbufsize;
	}

	if (m_cb)
	{
		m_out.data = m_outbuff;
		m_out.datasize = m_outbufsize;

		m_cb(&m_out);
	}

	++m_seqbase;

	return e_rtppkt_err_noerror;
}

int32_t rtp_packet::h264_fua(uint8_t* data, uint32_t datasize, bool last)
{
	uint32_t segmentlen = 0;
	uint8_t paddinglen = 0;
	uint8_t nal = 0;
	bool fuafirst = true, fualast = false;

	_rtp_fua_indicator fuaind;
	_rtp_fua_header fuahead;

	nal = data[0] & 0x1f;

	fuaind.f = (data[0] >> 7) & 0x01;
	fuaind.n = (data[0] >> 5) & 0x03;

	fuahead.t = nal;

	++data;
	--datasize;

	for (; datasize > 0;)
	{
		m_outbufsize = 0;

		segmentlen = (datasize >= (RTP_PAYLOAD_MAX_SIZE -2) ) ? (RTP_PAYLOAD_MAX_SIZE - 2) : datasize;
		fualast = (0 == (datasize - segmentlen));

		fuahead.s = (fuafirst ? 1 : 0);
		fuahead.e = (fualast ? 1 : 0);

		if (e_rtppkt_am_4octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 2) % 4) ? 0 : (4 - (sizeof(m_rtphead) + segmentlen + 2) % 4);
		}
		else if (e_rtppkt_am_8octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 2) % 8) ? 0 : (8 - (sizeof(m_rtphead) + segmentlen + 2) % 8);
		}
		else
		{
			paddinglen = 0;
		}

		m_rtphead.p = (0 == paddinglen) ? 0 : 1;
		m_rtphead.mark = ( (last && fualast) ? 1 : 0);
		m_rtphead.seq = ::htons(m_seqbase);
		m_rtphead.timestamp = ::htonl(m_ttbase);

		memcpy(m_outbuff + m_outbufsize, &m_rtphead, sizeof(m_rtphead));
		m_outbufsize += sizeof(m_rtphead);

		memcpy(m_outbuff + m_outbufsize, &fuaind, sizeof(fuaind));
		m_outbufsize += sizeof(fuaind);

		memcpy(m_outbuff + m_outbufsize, &fuahead, sizeof(fuahead));
		m_outbufsize += sizeof(fuahead);

		memcpy(m_outbuff + m_outbufsize, data, segmentlen);
		m_outbufsize += segmentlen;

		if (0 != paddinglen)
		{
			if (paddinglen - 1 > 0)
			{
				memset(m_outbuff + m_outbufsize, 0x00, paddinglen - 1);
				m_outbufsize += (paddinglen - 1);
			}

			memcpy(m_outbuff + m_outbufsize, &paddinglen, 1);
			++m_outbufsize;
		}

		if (m_cb)
		{
			m_out.data = m_outbuff;
			m_out.datasize = m_outbufsize;

			m_cb(&m_out);
		}

		++m_seqbase;

		data += segmentlen;
		datasize -= segmentlen;
		fuafirst = false;
	}

	return e_rtppkt_err_noerror;
}

int32_t rtp_packet::h264_stapa(uint8_t* data, uint32_t datasize, bool last)
{
	return e_rtppkt_err_unsupportedmethod;
}


int32_t rtp_packet::h265_fus(uint8_t* data, uint32_t datasize, bool last)
{
	uint32_t segmentlen = 0;
	uint8_t paddinglen = 0;
	uint8_t nal = 0;
	uint8_t plhdr[2] = { 0 };
	bool fusfirst = true, fuslast = false;

	plhdr[0] = 49; //fus
	plhdr[0] = plhdr[0] << 1;
	plhdr[0] |= (data[0] & 0x80);
	plhdr[0] |= (data[0] & 0x01);
	plhdr[1] = data[1];
	
	_rtp_fus_header fushead;

	nal = (data[0] >> 1) & 0x3f;

	fushead.t = nal;

	data += 2;
	datasize -= 2;

	for (; datasize > 0;)
	{
		m_outbufsize = 0;

		segmentlen = (datasize >= (RTP_PAYLOAD_MAX_SIZE - 3)) ? (RTP_PAYLOAD_MAX_SIZE - 3) : datasize;
		fuslast = (0 == (datasize - segmentlen));

		fushead.s = (fusfirst ? 1 : 0);
		fushead.e = (fuslast ? 1 : 0);

		if (e_rtppkt_am_4octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 3) % 4) ? 0 : (4 - (sizeof(m_rtphead) + segmentlen + 3) % 4);
		}
		else if (e_rtppkt_am_8octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 3) % 8) ? 0 : (8 - (sizeof(m_rtphead) + segmentlen + 3) % 8);
		}
		else
		{
			paddinglen = 0;
		}

		m_rtphead.p = (0 == paddinglen) ? 0 : 1;
		m_rtphead.mark = ((fuslast &&last) ? 1 : 0);
		m_rtphead.seq = ::htons(m_seqbase);
		m_rtphead.timestamp = ::htonl(m_ttbase);

		memcpy(m_outbuff + m_outbufsize, &m_rtphead, sizeof(m_rtphead));
		m_outbufsize += sizeof(m_rtphead);

		memcpy(m_outbuff + m_outbufsize, &plhdr, sizeof(plhdr));
		m_outbufsize += sizeof(plhdr);

		memcpy(m_outbuff + m_outbufsize, &fushead, sizeof(fushead));
		m_outbufsize += sizeof(fushead);

		memcpy(m_outbuff + m_outbufsize, data, segmentlen);
		m_outbufsize += segmentlen;

		if (0 != paddinglen)
		{
			if (paddinglen - 1 > 0)
			{
				memset(m_outbuff + m_outbufsize, 0x00, paddinglen - 1);
				m_outbufsize += (paddinglen - 1);
			}

			memcpy(m_outbuff + m_outbufsize, &paddinglen, 1);
			++m_outbufsize;
		}

		if (m_cb)
		{
			m_out.data = m_outbuff;
			m_out.datasize = m_outbufsize;

			m_cb(&m_out);
		}

		++m_seqbase;

		data += segmentlen;
		datasize -= segmentlen;
		fusfirst = false;
	}

	return e_rtppkt_err_noerror;
}

int32_t rtp_packet::h265_aps(uint8_t* data, uint32_t datasize, bool last)
{
	return e_rtppkt_err_unsupportedmethod;
}

int32_t rtp_packet::svacv_fua(uint8_t* data, uint32_t datasize, bool last)
{
	uint32_t segmentlen = 0;
	uint8_t paddinglen = 0;
	uint8_t nal = 0;
	bool fuafirst = true, fualast = false;

	_rtp_fua_indicator fuaind;
	_rtp_fua_header fuahead;

	nal = (data[0] >> 2) & 0x0f;

	fuaind.f = (data[0] >> 7) & 0x01;
	fuaind.n = (data[0] >> 6) & 0x01;

	fuahead.t = nal;

	++data;
	--datasize;

	for (; datasize > 0;)
	{
		m_outbufsize = 0;

		segmentlen = (datasize >= (RTP_PAYLOAD_MAX_SIZE - 2)) ? (RTP_PAYLOAD_MAX_SIZE - 2) : datasize;
		fualast = (0 == (datasize - segmentlen));

		fuahead.s = (fuafirst ? 1 : 0);
		fuahead.e = ( (fualast && last) ? 1 : 0);

		if (e_rtppkt_am_4octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 2) % 4) ? 0 : (4 - (sizeof(m_rtphead) + segmentlen + 2) % 4);
		}
		else if (e_rtppkt_am_8octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen + 2) % 8) ? 0 : (8 - (sizeof(m_rtphead) + segmentlen + 2) % 8);
		}
		else
		{
			paddinglen = 0;
		}

		m_rtphead.p = (0 == paddinglen) ? 0 : 1;
		m_rtphead.mark = (last ? 1 : 0);
		m_rtphead.seq = ::htons(m_seqbase);
		m_rtphead.timestamp = ::htonl(m_ttbase);

		memcpy(m_outbuff + m_outbufsize, &m_rtphead, sizeof(m_rtphead));
		m_outbufsize += sizeof(m_rtphead);

		memcpy(m_outbuff + m_outbufsize, &fuaind, sizeof(fuaind));
		m_outbufsize += sizeof(fuaind);

		memcpy(m_outbuff + m_outbufsize, &fuahead, sizeof(fuahead));
		m_outbufsize += sizeof(fuahead);

		memcpy(m_outbuff + m_outbufsize, data, segmentlen);
		m_outbufsize += segmentlen;

		if (0 != paddinglen)
		{
			if (paddinglen - 1 > 0)
			{
				memset(m_outbuff + m_outbufsize, 0x00, paddinglen - 1);
				m_outbufsize += (paddinglen - 1);
			}

			memcpy(m_outbuff + m_outbufsize, &paddinglen, 1);
			++m_outbufsize;
		}

		if (m_cb)
		{
			m_out.data = m_outbuff;
			m_out.datasize = m_outbufsize;

			m_cb(&m_out);
		}

		++m_seqbase;

		data += segmentlen;
		datasize -= segmentlen;
		fuafirst = false;
	}

	return e_rtppkt_err_noerror;
}

int32_t rtp_packet::svacv_stapa(uint8_t* data, uint32_t datasize, bool last)
{
	return e_rtppkt_err_unsupportedmethod;
}

int32_t rtp_packet::handle_common(uint8_t* data, uint32_t datasize)
{
	uint32_t segmentlen = 0;
	uint8_t paddinglen = 0;

	bool first = true;
	bool last = false;

	for (; datasize > 0;)
	{
		m_outbufsize = 0;

		segmentlen = (datasize >= RTP_PAYLOAD_MAX_SIZE) ? RTP_PAYLOAD_MAX_SIZE : datasize;
		last = (0 == (datasize - segmentlen));

		if (e_rtppkt_am_4octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen) % 4) ? 0 : (4 - (sizeof(m_rtphead) + segmentlen) % 4);
		}
		else if (e_rtppkt_am_8octet == m_attr.alimod)
		{
			paddinglen = (0 == (sizeof(m_rtphead) + segmentlen) % 8) ? 0 : (8 - (sizeof(m_rtphead) + segmentlen) % 8);
		}
		else
		{
			paddinglen = 0;
		}

		m_rtphead.p = (0 == paddinglen) ? 0 : 1;
		m_rtphead.mark = (e_rtppkt_mt_audio == m_attr.mediatype) ? ( first ? 1 : 0 ) : ( last ? 1 : 0);
		m_rtphead.seq = ::htons(m_seqbase);
		m_rtphead.timestamp = ::htonl(m_ttbase);

		memcpy(m_outbuff + m_outbufsize, &m_rtphead, sizeof(m_rtphead));
		m_outbufsize += sizeof(m_rtphead);

		memcpy(m_outbuff + m_outbufsize, data, segmentlen);
		m_outbufsize += segmentlen;

		if (0 != paddinglen)
		{
			if (paddinglen - 1 > 0)
			{
				memset(m_outbuff + m_outbufsize, 0x00, paddinglen - 1);
				m_outbufsize += (paddinglen - 1);
			}
			
			memcpy(m_outbuff + m_outbufsize, &paddinglen, 1);
			++m_outbufsize;
		}

		if (m_cb)
		{
			m_out.data = m_outbuff;
			m_out.datasize = m_outbufsize;

			m_cb(&m_out);
		}

		++m_seqbase;

		data += segmentlen;
		datasize -= segmentlen;
		first = false;
	}

	if (m_inTimestamp == 0)
		m_ttbase += m_attr.ttincre;
	else
		m_ttbase = m_inTimestamp;


	return e_rtppkt_err_noerror;
}

//aac打包
int32_t rtp_packet::aac_adts(uint8_t* data, uint32_t datasize)
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