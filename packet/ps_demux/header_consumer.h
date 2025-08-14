#pragma once

#include "consumer_base.h"


class header_consumer : public consumer_base
{
public:
	header_consumer();
	virtual ~header_consumer();

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen);

	virtual uint8_t* output(uint32_t& datasize);

	virtual void clean();
};

