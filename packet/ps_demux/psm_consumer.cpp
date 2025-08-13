#include "psm_consumer.h"
#include "ps_demux.h"

psm_consumer::psm_consumer()
{
}

psm_consumer::~psm_consumer()
{
}

int32_t psm_consumer::handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen)
{
	if (datasize < sizeof(_ps_psm))
	{
		return e_ps_dux_err_invalidpsm;
	}

	_ps_psm* head = reinterpret_cast<_ps_psm*>(data);

	handlelen = 4 + 2 + head->get_stream_map_length();

	if ((handlelen < (sizeof(_ps_psm))) || (datasize < handlelen))
	{
		return e_ps_dux_err_invalidpsm;
	}

	if (head->cni)
	{
		uint32_t offset = sizeof(_ps_psm) + head->get_stream_info_length() + 2;
		if (handlelen < offset)
		{
			return e_ps_dux_err_invalidpsm;
		}

		uint16_t esml = (data[sizeof(_ps_psm) + head->get_stream_info_length()] << 8 | data[sizeof(_ps_psm) + head->get_stream_info_length() + 1]);
		if (handlelen < (offset + esml))
		{
			return e_ps_dux_err_invalidpsm;
		}

		if (esml >= 4)
		{
			uint8_t* addr = data + offset;
			_ps_elementary_stream  si;

			for (uint32_t c = 0; c < (static_cast<uint32_t>(esml) - 3) ;)
			{
				si.st = addr[c];
				si.sid = addr[c + 1];
				si.set_stream_info_length((addr[c + 2] << 8) | addr[c + 3]);

				m_streaminfo.push_back(si);

				c += 4 + si.get_stream_info_length();
			}
		}
	}
	
	return e_ps_dux_err_noerror;
}

uint8_t* psm_consumer::output(uint32_t& datasize)
{
	datasize = 0;

	return NULL/*nullptr*/;
}

void psm_consumer::clean()
{
}

const std::vector<_ps_elementary_stream>& psm_consumer::get_streaminfo() const
{
	return m_streaminfo;
}