#include <cstring>
#include "pes_consumer.h"
#include "ps_def.h"
#include "ps_demux.h"

pes_consumer::pes_consumer(ps_demux_callback cb, void* userdata, int32_t mode, uint32_t h, uint8_t st, uint8_t sid)
	: m_datasize(0)
	, m_mode(mode)
	, m_cb(cb)
	, m_lastpts(0)
	, m_lastdts(0)
{
	m_out.handle = h;
	m_out.streamtype = st;
	m_out.streamid = sid;
	m_out.userdata = userdata;
}

pes_consumer::~pes_consumer()
{
}

int32_t pes_consumer::handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen)
{
	//pes check
	if (datasize < sizeof(_ps_pes))
	{
		m_datasize = 0;

		return e_ps_dux_err_invalidpes;
	}

	_ps_pes* head = reinterpret_cast<_ps_pes*>(data);

	handlelen = 4 + 2 + head->get_pes_packet_length();

	if ((datasize < handlelen) || (handlelen < (sizeof(_ps_pes) + head->phdl)))
	{
		m_datasize = 0;

		return e_ps_dux_err_invalidpes;
	}

	uint64_t pts = 0, dts = 0;
	_ps_pts* p = NULL/*nullptr*/;
	_ps_dts* d = NULL/*nullptr*/;
	uint8_t* addr = NULL/*nullptr*/;
	uint32_t len = 0;
	bool havepts = false;

	//handle pts dts
	switch (head->pdf)
	{

	case 0x00:
	{
	}break;

	case 1:
	{
		m_datasize = 0;

		return e_ps_dux_err_invalidptsdtsflag;

	}break;

	case 2:
	{
		if ((head->phdl < sizeof(_ps_pts)))
		{
			m_datasize = 0;

			return e_ps_dux_err_invalidpes;
		}

		p = reinterpret_cast<_ps_pts*>(data + sizeof(_ps_pes));

		pts = p->get();
		dts = pts;

		havepts = true;

	}break;

	case 3:
	{

		if ((head->phdl < (sizeof(_ps_pts) + sizeof(_ps_dts))))
		{
			m_datasize = 0;

			return e_ps_dux_err_invalidpes;
		}

		p = reinterpret_cast<_ps_pts*>(data + sizeof(_ps_pes));
		d = reinterpret_cast<_ps_pts*>(data + sizeof(_ps_pes) + sizeof(_ps_pts));

		pts = p->get();
		dts = d->get();

		havepts = true;

	}break;

	default:
	{
		m_datasize = 0;

		return e_ps_dux_err_invalidptsdtsflag;

	}break;

	}

	//output
	if ( (e_ps_dux_timestamp == m_mode) && (0 != head->pdf) && (m_lastpts != pts) && (m_datasize > 0))
	{
		if (m_cb)
		{
			m_out.pts = m_lastpts;
			m_out.dts = m_lastdts;
			m_out.data = m_data;
			m_out.datasize = m_datasize;

			m_cb(&m_out);
		}

		m_datasize = 0;
	}

	//handle media data
	if (havepts)
	{
		m_lastpts = pts;
		m_lastdts = dts;
	}


	len = handlelen - sizeof(_ps_pes) - head->phdl;
	addr = data + sizeof(_ps_pes) + head->phdl;

	if (PS_DEMUX_OUTPUT_BUFF_MAX_SIZE < (m_datasize + len))
	{
		m_datasize = 0;
	}
	else
	{
		memcpy(m_data + m_datasize, addr, len);
		m_datasize += len;
	}

	
	return e_ps_dux_err_noerror;
}

uint8_t* pes_consumer::output(uint32_t& datasize)
{
	if (e_ps_dux_timestamp == m_mode)
	{
		datasize = 0;

		return NULL/*nullptr*/;
	}
	else
	{
		datasize = m_datasize;

		return m_data;
	}
}

void pes_consumer::clean()
{
	m_datasize = 0;

	m_lastpts = 0;
	m_lastdts = 0;
}

uint64_t pes_consumer::get_lastpts() const
{
	return m_lastpts;
}

uint64_t pes_consumer::get_lastdts() const
{
	return m_lastdts;
}