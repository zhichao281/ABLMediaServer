#include <memory>
#include "session.h"
#include "common.h"
#include <malloc.h>

rtp_session_packet::rtp_session_packet(rtp_packet_callback cb, void* userdata)
	: m_id(generate_identifier_rtppacket())
	, m_cb(cb)
	, m_userdata(userdata)
{
}

rtp_session_packet::~rtp_session_packet()
{
	recycle_identifier_rtppacket(m_id);
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

uint32_t rtp_session_packet::get_id() const
{
	return m_id;
}

int32_t rtp_session_packet::handle(_rtp_packet_input* in)
{
	auto it = m_pktmap.find(in->ssrc);
	if (m_pktmap.end() == it)
	{
		return e_rtppkt_err_notfindpacket;
	}
	
	return it->second->handle(in->data, in->datasize,in->timestamp);
}

int32_t rtp_session_packet::set_option(_rtp_packet_sessionopt* opt)
{
	auto it = m_pktmap.find(opt->ssrc);
	if (m_pktmap.end() != it)
	{
		return e_rtppkt_err_existingssrc;
	}

	rtp_packet_ptr packet;

	try
	{
		_rtp_packet_sessionopt newopt = *opt;

		if ((e_rtppkt_mt_audio != newopt.mediatype) && (e_rtppkt_mt_video != newopt.mediatype))
		{
			newopt.mediatype = get_mediatype(newopt.streamtype);

			if ((e_rtppkt_mt_audio != newopt.mediatype) && (e_rtppkt_mt_video != newopt.mediatype))
			{
				return e_rtppkt_err_invalidmeidatype;
			}
		}

		if (0 == newopt.ttincre)
		{
			newopt.ttincre = (e_rtppkt_mt_audio == newopt.mediatype) ? 400 : 3600;
		}

		packet = std::make_shared<rtp_packet>(m_cb, const_cast<void*>(m_userdata), newopt);
	}
	catch (const std::bad_alloc& /*e*/)
	{
		return e_rtppkt_err_mallocpacketerror;
	}
	catch (...)
	{
		return e_rtppkt_err_mallocpacketerror;
	}

	auto ret = m_pktmap.insert(std::make_pair(opt->ssrc, packet));
	if (!ret.second)
	{
		return e_rtppkt_err_managerpacketerror;
	}


	return e_rtppkt_err_noerror;
}

int32_t rtp_session_packet::reset_option(_rtp_packet_sessionopt* opt)
{
	auto it = m_pktmap.find(opt->ssrc);
	if (m_pktmap.end() == it)
	{
		return e_rtppkt_err_nonexistingssrc;
	}
	else
	{
		m_pktmap.erase(it);
	}

	return e_rtppkt_err_noerror;
}