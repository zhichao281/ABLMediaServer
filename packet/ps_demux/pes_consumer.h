#ifndef _PS_MUX_DEMUX_PES_CONSUMER_H_
#define _PS_MUX_DEMUX_PES_CONSUMER_H_

#include "consumer_base.h"
#include "ps_demux.h"

class pes_consumer : public consumer_base
{
public:
	pes_consumer(ps_demux_callback cb, void* userdata, int32_t mode, uint32_t h, uint8_t st, uint8_t sid);
	virtual ~pes_consumer();

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen);

	virtual uint8_t* output(uint32_t& datasize);

	virtual void clean();

	uint64_t get_lastpts() const;

	uint64_t get_lastdts() const;

private:
	uint8_t m_data[PS_DEMUX_OUTPUT_BUFF_MAX_SIZE];
	uint32_t m_datasize;

	int32_t m_mode;
	ps_demux_callback m_cb;
	_ps_demux_cb m_out;

	uint64_t m_lastpts;
	uint64_t m_lastdts;
};
typedef std::shared_ptr<pes_consumer> pes_consumer_ptr;

#endif