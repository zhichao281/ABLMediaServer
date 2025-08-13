#include "rtp_packet.h"
#include "session_manager.h"
#include "session.h"

RTP_PACKET_API int32_t rtp_packet_start(rtp_packet_callback cb, void* userdata, uint32_t* h)
{
	if (!cb || !h)
	{
		return e_rtppkt_err_invalidparam;
	}

	*h = 0;

	rtp_session_ptr s = rtp_session_manager::getInstance().malloc(cb, userdata);
	if (!s)
	{
		return e_rtppkt_err_mallocsessionerror;
	}

	if (!rtp_session_manager::getInstance().push(s))
	{
		return e_rtppkt_err_managersessionerror;
	}

	*h = s->get_id();

	return e_rtppkt_err_noerror;

}

RTP_PACKET_API int32_t rtp_packet_stop(uint32_t h)
{
	if (!rtp_session_manager::getInstance().pop(h))
	{
		return e_rtppkt_err_invalidsessionhandle;
	}

	return e_rtppkt_err_noerror;
}

RTP_PACKET_API int32_t rtp_packet_input(_rtp_packet_input* input)
{
	if (!input || !input->data || (0 == input->datasize))
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = rtp_session_manager::getInstance().get(input->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->handle(input); 
}

RTP_PACKET_API int32_t rtp_packet_setsessionopt(_rtp_packet_sessionopt* opt)
{
	if (!opt)
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = rtp_session_manager::getInstance().get(opt->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->set_option(opt);
}

RTP_PACKET_API int32_t rtp_packet_resetsessionopt(_rtp_packet_sessionopt* opt)
{
	if (!opt)
	{
		return e_rtppkt_err_invalidparam;
	}

	rtp_session_ptr s = rtp_session_manager::getInstance().get(opt->handle);

	if (!s)
	{
		return e_rtppkt_err_notfindsession;
	}

	return s->reset_option(opt);
}