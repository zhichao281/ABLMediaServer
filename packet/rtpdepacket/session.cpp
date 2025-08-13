#include <memory>
#include <thread>
#include <chrono>
#include <malloc.h>

#include "session.h"
#include "rtp_def.h"
#include "rtp_depacket.h"
#include "common.h"

rtp_session::rtp_session(rtp_depacket_callback cb, void* userdata) 
	: m_id(generate_identifier())
	, m_cb(cb)
	, m_userdata(userdata) 
{

}

rtp_session::~rtp_session()
{
	recycle_identifier(m_id);
#ifndef _WIN32
	malloc_trim(0);
#endif // _WIN32
}

int32_t rtp_session::handle(uint8_t* data, uint32_t datasize)
{
	if (!data || (sizeof(_rtp_header) > datasize))
	{
		return e_rtpdepkt_err_invaliddata;
	}

	uint32_t ssrc = 0;
	if (!rtp_check(data, datasize, ssrc))
	{
		return e_rtpdepkt_err_malformedpacket;
	}

	rtp_depacket_ptr& dux = get_depacket(ssrc);
	if (!dux)
	{
		return e_rtpdepkt_err_notfinddepacket;
	}

	return dux->handle(data, datasize);
}

void rtp_session::set_payload(uint8_t payload, uint32_t streamtype)
{
	m_payloadMap[payload] = streamtype;

	for (auto it = m_depacketMap.begin(); m_depacketMap.end() != it; )
	{
		if (it->second)
		{
			it->second->set_payload(payload, streamtype);
		}
		else
		{
			m_depacketMap.erase(it++);
		}
	}
}

void rtp_session::set_mediaoption(const std::string& opt, const std::string& param)
{
	if (opt.empty())
	{
		return;
	}

	m_mediaoptMap[opt] = param;

	for (auto it = m_depacketMap.begin(); m_depacketMap.end() != it;)
	{
		if (it->second)
		{
			it->second->set_mediaoption(opt, param);
		}
		else
		{
			m_depacketMap.erase(it++);
		}
	}
}

bool rtp_session::rtp_check(uint8_t* data, uint32_t datasize, uint32_t& ssrc)
{
	_rtp_header* head = reinterpret_cast<_rtp_header*>(data);
	if (head)
	{
		if (RTP_VERSION != head->v)	//1.version check
		{
			return false;
		}

		if (is_rtcp(head->payload))	//2.payload check
		{
			return false;
		}

		uint32_t cclen = 0;
		uint32_t ehlen = 0;
		uint32_t paddinglen = 0;

		if (head->cc)	//3.csrc check
		{
			cclen = 4 * head->cc;

			if (datasize < (sizeof(_rtp_header) + cclen))
			{
				return false;
			}
		}

		if (head->x)	//4.extension header check
		{
			if (datasize < (sizeof(_rtp_header) + cclen + 4))
			{
				return false;
			}

			ehlen = 4 + ((data[sizeof(_rtp_header) + cclen + 2] << 8) | data[sizeof(_rtp_header) + cclen + 3]) * 4;

			if (datasize < (sizeof(_rtp_header) + cclen + ehlen))
			{
				return false;
			}

		}

		if (head->p) //5.padding octet check
		{
			if (datasize < (sizeof(_rtp_header) + cclen + ehlen + 1))
			{
				return false;
			}

			paddinglen  = static_cast<uint32_t>(data[datasize - 1]);
			if (0 == paddinglen)
			{
				return false;
			}

			if (datasize < (sizeof(_rtp_header) + cclen + ehlen + paddinglen))
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		return false;
	}

	return false;
}

bool rtp_session::is_rtcp(uint8_t payload)
{
	return ((e_rtcp_pkt_sr <= payload) && (e_rtcp_pkt_app >= payload));
}

rtp_depacket_ptr& rtp_session::get_depacket(uint32_t ssrc)
{
	auto it = m_depacketMap.find(ssrc);
	if (m_depacketMap.end() != it)
	{
		if (it->second)
		{
			return it->second;
		}
		else
		{
			m_depacketMap.erase(it);
		}
	}

	std::pair<std::unordered_map<uint32_t, rtp_depacket_ptr>::iterator, bool> ret;
	rtp_depacket_ptr dux;

	do 
	{
		try
		{
			dux = std::make_shared<rtp_depacket>(ssrc, reinterpret_cast<rtp_depacket_callback>(m_cb), const_cast<void*>(m_userdata), get_id());
		
			ret = m_depacketMap.insert(std::make_pair(ssrc, dux));

			if (!ret.second)
			{
				dux.reset();
			}

			for (std::unordered_map<uint8_t, uint32_t>::iterator it = m_payloadMap.begin(); m_payloadMap.end() != it; ++it)
			{
				dux->set_payload(it->first, it->second);
			}

			for (std::unordered_map<std::string, std::string>::iterator it = m_mediaoptMap.begin(); m_mediaoptMap.end() != it; ++it)
			{
				dux->set_mediaoption(it->first, it->second);
			}
		}
		catch (const std::bad_alloc& /*e*/)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}
		catch (...)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			continue;
		}

	} while (!dux);

	return ret.first->second;
}