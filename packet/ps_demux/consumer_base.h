#ifndef _PS_MUX_DEMUX_CONSUMER_BASE_H_
#define _PS_MUX_DEMUX_CONSUMER_BASE_H_


#include <stdint.h>
#include <memory>

#define PS_DEMUX_OUTPUT_BUFF_MAX_SIZE (4 * 1024 * 1024)

class consumer_base
{
public:
	consumer_base(){}
	virtual ~consumer_base() {}

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen) = 0;

	virtual uint8_t* output(uint32_t& datasize) = 0;

	virtual void clean() = 0;

};
typedef std::shared_ptr<consumer_base> consumer_base_ptr;

#endif