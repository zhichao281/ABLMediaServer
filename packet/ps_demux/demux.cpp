#include <stdexcept>
#include <memory>
#include "demux.h"
#include "common.h"
#include "ps_def.h"
#include "header_consumer.h"
#include "sysheader_consumer.h"
#include "psm_consumer.h"
#include "pes_consumer.h"
#include "pes_nonmedia_consumer.h"
#include <malloc.h>

ps_demux::ps_demux(ps_demux_callback cb, void* userdata, int32_t duxmode)
	: m_id(generate_identifier())
	, m_cb(cb)
	, m_userdata(userdata)
	, m_duxmode(duxmode)
{

}

ps_demux::~ps_demux()
{
	recycle_identifier(m_id);
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

int32_t ps_demux::handle(uint8_t* data, uint32_t datasize)
{
	if (!data || (0 == datasize))
	{
		return e_ps_dux_err_invaliddata;
	}

	int32_t ret = e_ps_dux_err_noerror;

	switch (m_duxmode)
	{
	case e_ps_dux_timestamp:
		ret = handle_by_timestamp(data, datasize);
		break;
	case e_ps_dux_packet:
		ret = handle_by_packet(data, datasize);
		break;
	default:
		ret = handle_by_timestamp(data, datasize);
		break;
	}

	return ret;
}

int32_t ps_demux::handle_by_timestamp(uint8_t* data, uint32_t datasize)
{
	uint8_t sid = 0;
	int32_t ret = e_ps_dux_err_noerror;
	uint32_t handlelen = 0;

	while (datasize)
	{
		if (check_startcode(data, datasize, sid))
		{
			if (sid < e_ps_segment_psheader)
			{
				return e_ps_dux_err_invalidstreamid;
			}

			try
			{
				consumer_base_ptr& consumer = get_consumer(sid, true);

				ret = consumer->handle(data, datasize, handlelen);

				if ((e_ps_dux_err_noerror != ret))
				{
					consumer->clean();

					return ret;
				}

				datasize -= handlelen;
				data += handlelen;

			}
			catch (const std::exception& /*e*/)
			{
				return e_ps_dux_err_notfindconsumer;
			}
			catch (...)
			{
				return e_ps_dux_err_notfindconsumer;
			}
		}
		else
		{
			return e_ps_dux_err_checkpsstartcodefail;
		}
	}

	return e_ps_dux_err_noerror;
}

int32_t ps_demux::handle_by_packet(uint8_t* data, uint32_t datasize)
{
	// check ps header
	uint8_t sid = 0;
	bool csr = check_startcode(data, datasize, sid);

	if (!csr || (e_ps_segment_psheader != sid))
	{
		return e_ps_dux_err_notfindpsheader;
	}

	int32_t ret = e_ps_dux_err_noerror;
	uint32_t handlelen = 0;

	//deal with ps data
	while (datasize)
	{
		if (check_startcode(data, datasize, sid))
		{
			if (sid < e_ps_segment_psheader)
			{
				return e_ps_dux_err_invalidstreamid;
			}

			try
			{
				consumer_base_ptr& consumer = get_consumer(sid, true);

				ret = consumer->handle(data, datasize, handlelen);

				if ( (e_ps_dux_err_noerror != ret))
				{
					consumer->clean();

					return ret;
				}
				
				datasize -= handlelen;
				data += handlelen;
				
			}
			catch (const std::exception& /*e*/)
			{
				return e_ps_dux_err_notfindconsumer;
			}
			catch (...)
			{
				return e_ps_dux_err_notfindconsumer;
			}
		}
		else
		{
			return e_ps_dux_err_checkpsstartcodefail;
		}
	}

	output();

	return e_ps_dux_err_noerror;
	
}

bool ps_demux::check_startcode(uint8_t* data, uint32_t datasize, uint8_t& sid)
{
	sid = 0;

	if (datasize < (sizeof(PS_START_CODE) / sizeof(uint8_t) + 1))
	{
		return false;
	}

	if (0 == memcmp(data, PS_START_CODE, sizeof(PS_START_CODE) / sizeof(uint8_t)))
	{
		sid = data[sizeof(PS_START_CODE) / sizeof(uint8_t)];

		return true;
	}

	return false;
}

consumer_base_ptr& ps_demux::get_consumer(uint8_t sid, bool create)
{
	auto it = m_consumermap.find(sid);
	if (m_consumermap.end() != it)
	{
		return it->second;
	}

	if (!create)
	{
		throw std::runtime_error("consumer is not existed");
	}

	consumer_base_ptr p;
	uint8_t st = 0;

	try
	{
		switch (sid)
		{
		case e_ps_segment_psheader:
			p = std::static_pointer_cast<consumer_base>(std::make_shared<header_consumer>());
			break;

		case e_ps_segment_pssysheader:
			p = std::static_pointer_cast<consumer_base>(std::make_shared<sysheader_consumer>());
			break;

		case e_ps_segment_pspsm:
			p = std::static_pointer_cast<consumer_base>(std::make_shared<psm_consumer>());
			break;

		case e_ps_streamid_private1:
			p = std::static_pointer_cast<consumer_base>(std::make_shared<pes_consumer>(m_cb, const_cast<void*>(m_userdata),
														m_duxmode, m_id, 0, e_ps_streamid_private1));

		case e_ps_streamid_padding:
		case e_ps_streamid_private2:
		case e_ps_streamid_ecm:
		case e_ps_streamid_emm:
		case e_ps_streamid_dsm :
		case e_ps_streamid_iso13522:
		case e_ps_streamid_psd:
			p = std::static_pointer_cast<consumer_base>(std::make_shared<pes_nonmedia_consumer>());
			break;

		default:
		{
			if ((e_ps_streamid_minaudio <= sid) && (e_ps_streamid_maxvideo >= sid))	//media stream id
			{
				if (get_streamtype(sid, st))
				{
					p = std::static_pointer_cast<consumer_base>(std::make_shared<pes_consumer>(m_cb, 
																				const_cast<void*>(m_userdata),
																				m_duxmode, m_id, st, sid));
				}
				else
				{
					throw std::runtime_error("not find psm information");
				}
			}
			else if ((e_ps_streamid_minreserved <= sid) && (e_ps_streamid_maxreserved))	//media stream id
			{
				p = std::static_pointer_cast<consumer_base>(std::make_shared<pes_nonmedia_consumer>());
			}
			else
			{
				throw std::runtime_error("illegal stream id");
			}

		}break;

		}
	}
	catch (const std::bad_alloc& /*e*/)
	{
		throw;
	}
	catch (...)
	{
		throw;
	}
	
	auto ret = m_consumermap.insert(std::make_pair(sid, p));
	if (!ret.second)
	{
		throw std::runtime_error("manage consumer failed");
	}

	return ret.first->second;
}

bool ps_demux::get_streamtype(uint8_t sid, uint8_t& st)
{
	try
	{
		psm_consumer_ptr p = std::dynamic_pointer_cast<psm_consumer>(get_consumer(e_ps_segment_pspsm, false));

		const std::vector<_ps_elementary_stream>& v = p->get_streaminfo();

		for (std::vector<_ps_elementary_stream>::const_iterator it = v.begin(); v.end() != it; ++it)
		{
			if (it->sid == sid)
			{
				st = it->st;

				return true;
			}
		}

		return false;
	}
	catch (const std::exception /*e*/)
	{
		return false;
	}
	catch (...)
	{
		return false;
	}
	
	return false;
}

void ps_demux::output()
{
	if (m_cb)
	{
		try
		{
			uint8_t* data = NULL/*nullptr*/;
			uint32_t datasize = 0;

			psm_consumer_ptr p = std::dynamic_pointer_cast<psm_consumer>(get_consumer(e_ps_segment_pspsm, false));

			const std::vector<_ps_elementary_stream>& v = p->get_streaminfo();

			for (std::vector<_ps_elementary_stream>::const_iterator it = v.begin(); v.end() != it; ++it)
			{
				pes_consumer_ptr c = std::dynamic_pointer_cast<pes_consumer>(get_consumer(it->sid, false));

				data = c->output(datasize);

				if (data && (datasize > 0))
				{
					m_out.handle = m_id;
					m_out.streamid = it->sid;
					m_out.streamtype = it->st;
					m_out.userdata = const_cast<void*>(m_userdata);
					m_out.pts = c->get_lastpts();
					m_out.dts = c->get_lastdts();
					m_out.data = data;
					m_out.datasize = datasize;

					m_cb(&m_out);

					c->clean();
				}
			}
		}
		catch (const std::exception /*e*/)
		{
		}
		catch (...)
		{
		}
	}
}