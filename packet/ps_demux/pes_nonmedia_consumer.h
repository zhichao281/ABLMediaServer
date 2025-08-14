#pragma once

#include "consumer_base.h"

class pes_nonmedia_consumer : public consumer_base
{
public:
	pes_nonmedia_consumer();
	virtual ~pes_nonmedia_consumer();

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen);

	virtual uint8_t* output(uint32_t& datasize);

	virtual void clean();
};

