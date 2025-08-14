#pragma once

#include <vector>
#include <memory>
#include "consumer_base.h"
#include "ps_def.h"

class psm_consumer : public consumer_base
{
public:
	psm_consumer();
	virtual ~psm_consumer();

	virtual int32_t handle(uint8_t* data, uint32_t datasize, uint32_t& handlelen);

	virtual uint8_t* output(uint32_t& datasize);

	virtual void clean();

	const std::vector<_ps_elementary_stream>&  get_streaminfo() const;

private:
	std::vector<_ps_elementary_stream> m_streaminfo;
};
typedef std::shared_ptr<psm_consumer> psm_consumer_ptr;

