#include "pes_nonmedia_consumer.h"
#include "ps_def.h"
#include "ps_demux.h"


pes_nonmedia_consumer::pes_nonmedia_consumer()
{
}

pes_nonmedia_consumer::~pes_nonmedia_consumer()
{
}

int32_t pes_nonmedia_consumer::handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen)
{
	if (datasize < 6)
	{
		return e_ps_dux_err_invalidnonmediapes;
	}

	_ps_pes* head = reinterpret_cast<_ps_pes*>(data);

	handlelen = 4 + 2 + head->get_pes_packet_length();

	if (datasize < handlelen)
	{
		return e_ps_dux_err_invalidnonmediapes;
	}

	return e_ps_dux_err_noerror;
}

uint8_t* pes_nonmedia_consumer::output(uint32_t& datasize)
{
	datasize = 0;

	return NULL/*nullptr*/;
}

void pes_nonmedia_consumer::clean()
{
}