#include "header_consumer.h"
#include "ps_def.h"
#include "ps_demux.h"

header_consumer::header_consumer()
{
}

header_consumer::~header_consumer()
{
}

int32_t header_consumer::handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen)
{
	if (datasize < sizeof(_ps_header))
	{
		return e_ps_dux_err_invalidpsheader;
	}

	_ps_header* head = reinterpret_cast<_ps_header*>(data);

	handlelen = (sizeof(_ps_header) + head->psl);

	if (datasize < handlelen)
	{
		return e_ps_dux_err_invalidpsheader;
	}

	return e_ps_dux_err_noerror;
}

uint8_t* header_consumer::output(uint32_t& datasize)
{
	datasize = 0;
	
	return NULL/*nullptr*/;
}

void header_consumer::clean()
{
}