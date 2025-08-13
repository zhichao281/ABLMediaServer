#ifndef _PS_MUX_DEMUX_SYSHEADER_CONSUMER_H_
#define _PS_MUX_DEMUX_SYSHEADER_CONSUMER_H_

#include "consumer_base.h"

class sysheader_consumer : public consumer_base
{
public:
	sysheader_consumer();
	virtual ~sysheader_consumer();

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen);

	virtual uint8_t* output(uint32_t& datasize);

	virtual void clean();
};

#endif