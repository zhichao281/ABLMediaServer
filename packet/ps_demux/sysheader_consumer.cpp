#include "sysheader_consumer.h"
#include "ps_def.h"
#include "ps_demux.h"

sysheader_consumer::sysheader_consumer()
{
}

sysheader_consumer::~sysheader_consumer()
{
}

int32_t sysheader_consumer::handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen)
{
	if (datasize < sizeof(_ps_sys_header))
	{
		return e_ps_dux_err_invalidpssystemheader;
	}

	_ps_sys_header* head = reinterpret_cast<_ps_sys_header*>(data);

	handlelen = 4 + 2 + head->get_header_length();

	if ((handlelen < sizeof(_ps_sys_header)) || (datasize < handlelen))
	{
		return e_ps_dux_err_invalidpssystemheader;
	}

	return e_ps_dux_err_noerror;
}

uint8_t* sysheader_consumer::output(uint32_t& datasize)
{
	datasize = 0;

	return NULL/*nullptr*/;
}

void sysheader_consumer::clean()
{
}