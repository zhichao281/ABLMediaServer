
#ifdef USE_BOOST
#include <string.h>
#include "circular_buffer.h"


circular_buffer::circular_buffer()
	: m_capacity(0)
	, m_buffsize(0)
	, m_read(0)
	, m_write(0)
	, m_writecount(0)
	, m_commitcount(0)
{

}

circular_buffer::~circular_buffer()
{
	uninit();
}

bool circular_buffer::init(uint32_t capacity)
{
	if (m_buffer)
	{
		return true;
	}

	if (capacity > 0)
	{
		m_buffer.reset(new(std::nothrow) uint8_t[capacity]);
		if (m_buffer)
		{
			m_capacity = capacity;
			m_buffsize = 0;
			m_read = 0;
			m_write = 0;
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

uint32_t circular_buffer::write(const uint8_t* data, uint32_t datasize)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (m_buffer && data && (m_capacity >= (m_buffsize + datasize)))
	{
		if (m_capacity >= (m_write + datasize))
		{
			memcpy(m_buffer.get() + m_write, data, datasize);
		}
		else
		{
			memcpy(m_buffer.get() + m_write, data, m_capacity - m_write);
			memcpy(m_buffer.get(), data + (m_capacity - m_write), datasize - (m_capacity - m_write));
		}

		m_write = (m_write + datasize) % m_capacity;
		m_buffsize += datasize;
		m_writecount += datasize;

		return datasize;
	}

	return 0;
}

uint8_t* circular_buffer::try_read(uint32_t wanna, uint32_t& read)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	uint8_t* addr = NULL;
	read = 0;

	if ((m_buffsize > 0) && (wanna > 0))
	{
		if (m_write > m_read)
		{
			read = m_write - m_read;
		}
		else
		{
			read = m_capacity - m_read;
		}

		if (read > wanna)
		{
			read = wanna;
		}

		addr = m_buffer.get() + m_read;
	}

	return addr;
}

void circular_buffer::read_commit(uint32_t commit)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_mutex);
#endif

	if (commit > 0 && (m_buffsize <= commit))
	{
		m_commitcount += commit;
		m_buffsize -= commit;
		m_read = (m_read + commit) % m_capacity;
	}
}



#else

#include <string.h>
#include "circular_buffer.h"


circular_buffer::circular_buffer()
	: m_capacity(0)
	, m_buffsize(0)
	, m_read(0)
	, m_write(0)
	, m_writecount(0)
	, m_commitcount(0)
{

}

circular_buffer::~circular_buffer()
{
	uninit();
}

bool circular_buffer::init(uint32_t capacity)
{
	if (m_buffer)
	{
		return true;
	}

	if (capacity > 0)
	{
		m_buffer.reset(new(std::nothrow) uint8_t[capacity]);
		if (m_buffer)
		{
			m_capacity = capacity;
			m_buffsize = 0;
			m_read = 0;
			m_write = 0;
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

uint32_t circular_buffer::write(const uint8_t* data, uint32_t datasize)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	std::unique_lock<std::mutex> _lock(m_mutex);
#endif

	if (m_buffer && data && (m_capacity >= (m_buffsize + datasize)))
	{
		if (m_capacity >= (m_write + datasize))
		{
			memcpy(m_buffer.get() + m_write, data, datasize);
		}
		else
		{
			memcpy(m_buffer.get() + m_write, data, m_capacity - m_write);
			memcpy(m_buffer.get(), data + (m_capacity - m_write), datasize - (m_capacity - m_write));
		}

		m_write = (m_write + datasize) % m_capacity;
		m_buffsize += datasize;
		m_writecount += datasize;

		return datasize;
	}

	return 0;
}

uint8_t* circular_buffer::try_read(uint32_t wanna, uint32_t& read)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	std::unique_lock<std::mutex> _lock(m_mutex);
#endif

	uint8_t* addr = NULL;
	read = 0;

	if ((m_buffsize > 0) && (wanna > 0))
	{
		if (m_write > m_read)
		{
			read = m_write - m_read;
		}
		else
		{
			read = m_capacity - m_read;
		}

		if (read > wanna)
		{
			read = wanna;
		}

		addr = m_buffer.get() + m_read;
	}

	return addr;
}

void circular_buffer::read_commit(uint32_t commit)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_mutex);
#else
	std::unique_lock<std::mutex> _lock(m_mutex);
#endif

	if (commit > 0 && (m_buffsize <= commit))
	{
		m_commitcount += commit;
		m_buffsize -= commit;
		m_read = (m_read + commit) % m_capacity;
	}
}


#endif
