#include <malloc.h>
#include <cstring>

#include "mux.h"
#include "common.h"

ps_mux::ps_mux(ps_mux_callback cb, void* userdata, int32_t alignmode, int32_t ttmode, int32_t ttincre)
	: m_id(generate_identifier())
	, m_outbufsize(0)
	, m_sidbasev(0xe0)
	, m_sidbasea(0xc0)
	, m_cb(cb)
	, m_alimod(alignmode)
	, m_ttmod(ttmode)
	, m_ttincre(ttincre)
	, m_ttbase(0)
	, m_ttincrea(800)
	, m_ttbasea(0)
	, have_audio_cache_(false)
	, auido_cache_count_(0)
{
	if (0 == m_ttincre)
	{
		m_ttincre = 3600;
	}

	m_out.handle = get_id();
	m_out.userdata = userdata;

	m_psh.set_program_mux_rate(20000);	//8Mbps
	m_pssys.set_rate_bound(30000);	//12Mbps

	//add_pstd(e_psmux_mt_audio, 0xb8);
	//add_pstd(e_psmux_mt_video, 0xb9);
	//add_pstd(e_psmux_mt_unknown, 0xbd);
}

ps_mux::~ps_mux()
{
	recycle_identifier(m_id);
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

uint32_t ps_mux::get_id() const
{
	return m_id;
}

int32_t ps_mux::handle(const _ps_mux_input* in)
{
	m_outbufsize = 0;		//reset out buffer

	if (!in || !in->data || (0 == in->datasize))
	{
		return e_ps_mux_err_invaliddata;
	}

	if (e_ps_streamid_private1 == in->streamid)
	{
		add_stream(e_psmux_mt_unknown, in->streamtype, in->streamid);

		return produce_private(in);
	}


	int32_t mt = ((e_psmux_mt_video == in->mediatype) 
					|| (e_psmux_mt_audio == in->mediatype)) ? in->mediatype : get_mediatype(in->streamtype);

	switch (mt)
	{
	case  e_psmux_mt_video:
	{
		add_stream(e_psmux_mt_video, in->streamtype, in->streamid);

	}break;

	case e_psmux_mt_audio:
	{
		add_stream(e_psmux_mt_audio, in->streamtype, in->streamid);

		return produce_pesa(in);

	}break;

	default:
	{
		return e_ps_mux_err_invalidmediatype;

	}break;

	}

	
	int32_t ft = ((e_psmux_ft_i == in->frametype) ||
					(e_psmux_ft_p == in->frametype) ||
					(e_psmux_ft_b == in->frametype)) ? in->frametype  : get_frametype(in->streamtype, in->data, in->datasize);

	switch (ft)
	{
	case e_psmux_ft_i:
	{
		if (e_ps_mux_err_noerror != produce_psheader(in->pts))
		{
			return e_ps_mux_err_producepsheadererror;
		}

		if (e_ps_mux_err_noerror != produce_pssysheader())
		{
			return e_ps_mux_err_producepssysheadererror;
		}

		if (e_ps_mux_err_noerror != produce_psm())
		{
			return e_ps_mux_err_producepspsmerror;
		}

	}break;

	default:
	{
		if (e_ps_mux_err_noerror != produce_psheader(in->pts))
		{
			return e_ps_mux_err_producepsheadererror;
		}

	}break;

	}

	if (e_ps_mux_err_noerror != produce_pesv(in))
	{
		return e_ps_mux_err_producepesverror;
	}

	if (m_cb)
	{
		m_out.data = m_outbuf;
		m_out.datasize = m_outbufsize;

		m_cb(&m_out);
	}

	if (e_psmux_ttm_quantity != m_ttmod)
	{
		m_ttbase += m_ttincre;
	}

	have_audio_cache_ = false;
	auido_cache_count_ = 0;

	return e_ps_mux_err_noerror;
}

void ps_mux::add_stream(int32_t mt, uint8_t st, uint8_t sid)
{
	std::unordered_map<uint8_t, uint8_t>::iterator it = m_stsidmap.find(st);
	if (m_stsidmap.end() != it)
	{
		return;
	}

	switch (mt)
	{
	case  e_psmux_mt_video:
	{
		if ((sid < e_ps_streamid_minvideo) || (sid > e_ps_streamid_maxvideo))
		{
			sid = generate_video_sid();
		}
		
		m_stsidmap.insert(std::make_pair(st, sid));

		add_pstd(mt, sid);

	}break;

	case e_psmux_mt_audio:
	{
		if ((sid < e_ps_streamid_minaudio) || (sid > e_ps_streamid_maxaudio))
		{
			sid = generate_audio_sid();
		}

		m_stsidmap.insert(std::make_pair(st, sid));

		add_pstd(mt, sid);

	}break;

	default:
	{
		if (((sid >= e_ps_streamid_private1) && (sid < e_ps_streamid_minaudio)) ||
			((sid > e_ps_streamid_maxvideo) && (sid <= e_ps_streamid_psd)))
		{
			m_stsidmap.insert(std::make_pair(st, sid));

			add_pstd(mt, sid);
		}

	}break;

	}
}

void ps_mux::add_pstd(int32_t mt, uint8_t sid)
{
	_ps_pstd pstd;
	pstd.sid = sid;

	switch (mt)
	{
	case  e_psmux_mt_video:
	{
		pstd.bbs = 1;
		pstd.set_pstd_buffer_size_bound(800);

	}break;

	case e_psmux_mt_audio:
	{
		pstd.bbs = 0;
		pstd.set_pstd_buffer_size_bound(512);

	}break;

	default:
	{
		pstd.bbs = 1;
		pstd.set_pstd_buffer_size_bound(512);

	}break;

	}

	m_pstd.push_back(pstd);
}

uint8_t ps_mux::generate_video_sid()
{
	bool find = false;

	while (true)
	{
		for (std::unordered_map<uint8_t, uint8_t>::iterator it = m_stsidmap.begin(); m_stsidmap.end() != it; ++it)
		{
			if (it->second == m_sidbasev)
			{
				find = true;
				break;
			}
		}

		if (find)
		{
			++m_sidbasev;
			find = false;
			continue;
		}
		else
		{
			break;
		}
	}

	return m_sidbasev++;
}

uint8_t ps_mux::generate_audio_sid()
{
	bool find = false;

	while (true)
	{
		for (std::unordered_map<uint8_t, uint8_t>::iterator it = m_stsidmap.begin(); m_stsidmap.end() != it; ++it)
		{
			if (it->second == m_sidbasea)
			{
				find = true;
				break;
			}
		}

		if (find)
		{
			++m_sidbasea;
			find = false;
			continue;
		}
		else
		{
			break;
		}
	}

	return m_sidbasea++;
}

int32_t ps_mux::produce_psheader(uint64_t timestamp)
{
	if ((sizeof(m_psh) + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	switch (m_ttmod)
	{
	case e_psmux_ttm_quantity:
	{
		m_psh.set_system_clock_reference_base(timestamp);

	}break;

	default:
	{
		m_psh.set_system_clock_reference_base(m_ttbase);

	}break;

	}

	m_psh.set_system_clock_reference_extension(0);

	memcpy(m_outbuf + m_outbufsize, &m_psh, sizeof(m_psh));
	m_outbufsize += sizeof(m_psh);

	return e_ps_mux_err_noerror;
}

int32_t ps_mux::produce_pssysheader()
{
	uint32_t len = sizeof(m_pssys);
	len += sizeof(_ps_pstd) * static_cast<uint32_t>(m_pstd.size());

	if ((len + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	m_pssys.set_header_length(len - 6);

	memcpy(m_outbuf + m_outbufsize, &m_pssys, sizeof(m_pssys));
	m_outbufsize += sizeof(m_pssys);

	for (std::vector<_ps_pstd>::iterator it = m_pstd.begin(); m_pstd.end() != it; ++it)
	{
		memcpy(m_outbuf + m_outbufsize, &(*it), sizeof(_ps_pstd));
		m_outbufsize += sizeof(_ps_pstd);
	}

	return e_ps_mux_err_noerror;
}

int32_t ps_mux::produce_psm()
{
	uint32_t len = sizeof(m_psm);
	len += (2 + sizeof(_ps_elementary_stream) * static_cast<uint32_t>(m_stsidmap.size()) + 4);

	if ((len + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	m_psm.set_stream_map_length(len - 6);
	m_psm.set_stream_info_length(0);

	memcpy(m_outbuf + m_outbufsize, &m_psm, sizeof(m_psm));
	m_outbufsize += sizeof(m_psm);

	uint16_t l = sizeof(_ps_elementary_stream) * static_cast<uint16_t>(m_stsidmap.size());
	uint8_t esml[2] = { 0 };
	esml[0] = (l >> 8) & 0xff;
	esml[1] = l & 0xff;

	memcpy(m_outbuf + m_outbufsize, esml, sizeof(esml));
	m_outbufsize += sizeof(esml);

	_ps_elementary_stream es;

	for (std::unordered_map<uint8_t, uint8_t>::iterator it = m_stsidmap.begin(); m_stsidmap.end() != it; ++it)
	{
		es.st = it->first;
		es.sid = it->second; 

		memcpy(m_outbuf + m_outbufsize, &es, sizeof(es));
		m_outbufsize += sizeof(es);
	}

// 	m_outbuf[m_outbufsize + 0] = 0xf4;
// 	m_outbuf[m_outbufsize + 1] = 0xdc;
// 	m_outbuf[m_outbufsize + 2] = 0xbd;
// 	m_outbuf[m_outbufsize + 3] = 0x45;
	memset(m_outbuf + m_outbufsize, 0x00, 4);
	m_outbufsize += 4;

	return e_ps_mux_err_noerror;
}

int32_t ps_mux::produce_private(const _ps_mux_input* in)
{
	uint32_t segment = (in->datasize + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE - 1) / PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
	uint32_t lastseglen = in->datasize - (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

	uint32_t len = sizeof(m_pes) * segment;
	len += (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE + lastseglen;
	len += sizeof(_ps_pts) + sizeof(_ps_dts);

	if ((len + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	m_pes.sid = 0xbd;
	bool firstseg = true;
	uint32_t ppl = 0;
	uint32_t stufflen = 0;
	uint32_t addprefixlen = 0;
	_ps_pts pts;
	_ps_dts dts;

	for (uint32_t c = 1; c <= segment; ++c)
	{
		ppl = sizeof(m_pes) - 6;

		if (firstseg)
		{
			if (e_psmux_ttm_quantity == m_ttmod)
			{
				m_pes.pdf = (0 != in->dts) ? 3 : 2;
				m_pes.phdl = (3 == m_pes.pdf) ? (sizeof(_ps_pts) + sizeof(_ps_dts)) : sizeof(_ps_pts);

				if (3 == m_pes.pdf)
				{
					pts.fix = 3;
				}
				else
				{
					pts.fix = 2;
				}

				dts.fix = 1;

				pts.set(in->pts);
				dts.set(in->dts);
			}
			else
			{
				m_pes.pdf = 2;
				m_pes.phdl = sizeof(_ps_pts);

				pts.fix = 2;
				pts.set(m_ttbase);
			}

			firstseg = false;

			addprefixlen = m_outbufsize;
		}
		else
		{
			m_pes.pdf = 0;
			m_pes.phdl = 0;

			addprefixlen = 0;
		}

		ppl += m_pes.phdl;
		ppl += (c == segment) ? lastseglen : PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

		if (e_psmux_am_4octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 4)) ? 0 : (4 - ((ppl + addprefixlen + 6) % 4));
		}
		else if (e_psmux_am_8octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 8)) ? 0 : (8 - ((ppl + addprefixlen + 6) % 8));
		}
		else
		{
			stufflen = 0;
		}

		m_pes.phdl += stufflen;
		m_pes.set_pes_packet_length(ppl + stufflen);

		memcpy(m_outbuf + m_outbufsize, &m_pes, sizeof(m_pes));
		m_outbufsize += sizeof(m_pes);

		if (3 == m_pes.pdf)
		{
			memcpy(m_outbuf + m_outbufsize, &pts, sizeof(pts));
			m_outbufsize += sizeof(pts);

			memcpy(m_outbuf + m_outbufsize, &dts, sizeof(dts));
			m_outbufsize += sizeof(dts);
		}
		else if (2 == m_pes.pdf)
		{
			memcpy(m_outbuf + m_outbufsize, &pts, sizeof(pts));
			m_outbufsize += sizeof(pts);
		}
		else
		{
			;
		}

		if (stufflen > 0)
		{
			memset(m_outbuf + m_outbufsize, 0xff, stufflen);
			m_outbufsize += stufflen;
		}

		if (c != segment)
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), PS_MUX_PER_PES_PAYLOAD_MAX_SIZE);
			m_outbufsize += PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
		}
		else
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), lastseglen);
			m_outbufsize += lastseglen;
		}
	}


	if (m_cb)
	{
		m_out.data = m_outbuf;
		m_out.datasize = m_outbufsize;

		m_cb(&m_out);
	}

	return e_ps_mux_err_noerror;
}

int32_t ps_mux::produce_pesv(const _ps_mux_input* in)
{
	uint32_t segment = (in->datasize + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE - 1) / PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
	uint32_t lastseglen = in->datasize - (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

	uint32_t len = sizeof(m_pes) * segment;
	len += (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE + lastseglen;
	len += sizeof(_ps_pts) + sizeof(_ps_dts);

	if ((len + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	m_pes.sid = get_sid(e_psmux_mt_video, in->streamtype, in->streamid);
	bool firstseg = true;
	uint32_t ppl = 0;
	uint32_t stufflen = 0;
	uint32_t addprefixlen = 0;
	_ps_pts pts;
	_ps_dts dts;

	for (uint32_t c = 1; c <= segment; ++c)
	{
		ppl = sizeof(m_pes) - 6;

		if (firstseg)
		{
			if (e_psmux_ttm_quantity == m_ttmod)
			{
				m_pes.pdf = (0 != in->dts) ? 3 : 2;
				m_pes.phdl = (3 == m_pes.pdf) ? (sizeof(_ps_pts) + sizeof(_ps_dts)) : sizeof(_ps_pts);

				if (3 == m_pes.pdf)
				{
					pts.fix = 3;
				}
				else
				{
					pts.fix = 2;
				}

				dts.fix = 1;

				pts.set(in->pts);
				dts.set(in->dts);
			}
			else
			{
				m_pes.pdf = 2;
				m_pes.phdl = sizeof(_ps_pts);

				pts.fix = 2;
				pts.set(m_ttbase);
			}

			firstseg = false;

			addprefixlen = m_outbufsize;
		}
		else
		{
			m_pes.pdf = 0;
			m_pes.phdl = 0;

			addprefixlen = 0;
		}

		ppl += m_pes.phdl;
		ppl += (c == segment) ? lastseglen : PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

		if (e_psmux_am_4octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 4)) ? 0 :(4 - ((ppl + addprefixlen + 6) % 4));
		}
		else if (e_psmux_am_8octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 8)) ? 0 : (8 - ((ppl + addprefixlen + 6) % 8));
		}
		else
		{
			stufflen = 0;
		}

		m_pes.phdl += stufflen;
		m_pes.set_pes_packet_length(ppl + stufflen);

		memcpy(m_outbuf + m_outbufsize, &m_pes, sizeof(m_pes));
		m_outbufsize += sizeof(m_pes);

		if (3 == m_pes.pdf)
		{
			memcpy(m_outbuf + m_outbufsize, &pts, sizeof(pts));
			m_outbufsize += sizeof(pts);

			memcpy(m_outbuf + m_outbufsize, &dts, sizeof(dts));
			m_outbufsize += sizeof(dts);
		}
		else if (2 == m_pes.pdf)
		{
			memcpy(m_outbuf + m_outbufsize, &pts, sizeof(pts));
			m_outbufsize += sizeof(pts);
		}
		else
		{
			;
		}

		if (stufflen > 0)
		{
			memset(m_outbuf + m_outbufsize, 0xff, stufflen);
			m_outbufsize += stufflen;
		}

		if (c != segment)
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), PS_MUX_PER_PES_PAYLOAD_MAX_SIZE);
			m_outbufsize += PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
		}
		else
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), lastseglen);
			m_outbufsize += lastseglen;
		}
	}

	return e_ps_mux_err_noerror;
}

int32_t ps_mux::produce_pesa(const _ps_mux_input* in)
{
	uint32_t segment = (in->datasize + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE - 1) / PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
	uint32_t lastseglen = in->datasize - (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

	uint32_t len = sizeof(m_pes) * segment;
	len += (segment - 1) * PS_MUX_PER_PES_PAYLOAD_MAX_SIZE + lastseglen;
	len += sizeof(_ps_pts) + sizeof(_ps_dts);

	if ((len + m_outbufsize) > PS_MUX_OUTPUT_BUFFER_MAX_SIZE)
	{
		return e_ps_mux_err_outbufferisfull;
	}

	m_pes.sid = get_sid(e_psmux_mt_audio, in->streamtype, 0x00);
	bool firstseg = true;
	uint32_t ppl = 0;
	uint32_t stufflen = 0;
	uint32_t addprefixlen = 0;
	_ps_pts pts;

	for (uint32_t c = 1; c <= segment; ++c)
	{
		ppl = sizeof(m_pes) - 6;

		if (firstseg)
		{
			m_pes.pdf = 2;
			m_pes.phdl = sizeof(_ps_pts);
			pts.fix = 2;

			if (e_psmux_ttm_quantity == m_ttmod)
			{
				pts.set(in->pts);
			}
			else
			{
				pts.set(m_ttbasea);
			}

			firstseg = false;

			addprefixlen = m_outbufsize;
		}
		else
		{
			m_pes.pdf = 0;
			m_pes.phdl = 0;

			addprefixlen = 0;
		}

		ppl += m_pes.phdl;
		ppl += (c == segment) ? lastseglen : PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;

		if (e_psmux_am_4octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 4)) ? 0 : (4 - ((ppl + addprefixlen + 6) % 4));
		}
		else if (e_psmux_am_8octet == m_alimod)
		{
			stufflen = (0 == ((ppl + addprefixlen + 6) % 8)) ? 0 : (8 - ((ppl + addprefixlen + 6) % 8));
		}
		else
		{
			stufflen = 0;
		}

		m_pes.phdl += stufflen;
		m_pes.set_pes_packet_length(ppl + stufflen);

		memcpy(m_outbuf + m_outbufsize, &m_pes, sizeof(m_pes));
		m_outbufsize += sizeof(m_pes);

		if (2 == m_pes.pdf)
		{
			memcpy(m_outbuf + m_outbufsize, &pts, sizeof(pts));
			m_outbufsize += sizeof(pts);
		}
		else
		{
			;
		}

		if (stufflen > 0)
		{
			memset(m_outbuf + m_outbufsize, 0xff, stufflen);
			m_outbufsize += stufflen;
		}

		if (c != segment)
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), PS_MUX_PER_PES_PAYLOAD_MAX_SIZE);
			m_outbufsize += PS_MUX_PER_PES_PAYLOAD_MAX_SIZE;
		}
		else
		{
			memcpy(m_outbuf + m_outbufsize, in->data + PS_MUX_PER_PES_PAYLOAD_MAX_SIZE * (c - 1), lastseglen);
			m_outbufsize += lastseglen;
		}
	}

	if (m_cb)
	{
		m_out.data = m_outbuf;
		m_out.datasize = m_outbufsize;

		m_cb(&m_out);
	}

	m_ttbasea += m_ttincrea;

	return e_ps_mux_err_noerror;
}

uint8_t ps_mux::get_sid(int32_t mt, uint8_t st, uint8_t sid)
{
	uint8_t streamid = 0x00;

	auto it = m_stsidmap.find(st);
	if (m_stsidmap.end() != it)
	{
		return it->second;
	}

	if (e_psmux_mt_video == mt)
	{
		if ((sid >= e_ps_streamid_minvideo) && (sid <= e_ps_streamid_maxvideo))
		{
			streamid = sid;
		}
		else
		{
			streamid = generate_video_sid();
			
		}

		m_stsidmap.insert(std::make_pair(st, streamid));
	}
	else if (e_psmux_mt_audio == mt)
	{
		if ((sid >= e_ps_streamid_minaudio) && (sid <= e_ps_streamid_maxaudio))
		{
			streamid = sid;
		}
		else
		{
			streamid = generate_audio_sid();

		}

		m_stsidmap.insert(std::make_pair(st, streamid));
	}

	return streamid;
}