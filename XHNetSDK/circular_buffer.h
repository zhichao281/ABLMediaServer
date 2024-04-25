#pragma once 
#ifdef USE_BOOST


#include <stdint.h>
#include <boost/shared_array.hpp>
#include "auto_lock.h"

class circular_buffer
{
public:
	circular_buffer();
	~circular_buffer();

	bool init(uint32_t capacity);
	bool is_init();
	void uninit();

	uint32_t write(const uint8_t* data, uint32_t datasize);
	uint8_t* try_read(uint32_t wanna, uint32_t& read);
	void read_commit(uint32_t commit);
	uint32_t get_write_count();
	uint32_t get_commit_count();

private:
	circular_buffer(const circular_buffer&);
	circular_buffer& operator=(const circular_buffer&);

private:
	boost::shared_array<uint8_t> m_buffer;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
	uint32_t m_capacity;
	uint32_t m_buffsize;
	uint32_t m_read;
	uint32_t m_write;
	uint32_t m_writecount;
	uint32_t m_commitcount;
};


inline bool circular_buffer::is_init()
{
	return static_cast<bool>(m_buffer);
}

inline void circular_buffer::uninit()
{
	if (m_buffer)
	{
		m_buffer.reset();
	}

	m_buffsize = 0;
	m_read = 0;
	m_write = 0;
}

inline uint32_t circular_buffer::get_write_count()
{
	return m_writecount;
}

inline uint32_t circular_buffer::get_commit_count()
{
	return m_commitcount;
}

#else

#include <stdint.h>
#include "auto_lock.h"
#include <map>
#include <vector>
#include <memory>
#include <mutex>

class circular_buffer
{
public:
	circular_buffer();
	~circular_buffer();

	bool init(uint32_t capacity);
	bool is_init();
	void uninit();

	uint32_t write(const uint8_t* data, uint32_t datasize);
	uint8_t* try_read(uint32_t wanna, uint32_t& read);
	void read_commit(uint32_t commit);
	uint32_t get_write_count();
	uint32_t get_commit_count();

private:
	circular_buffer(const circular_buffer&);
	circular_buffer& operator=(const circular_buffer&);

private:
	std::unique_ptr<uint8_t[]> m_buffer;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	std::mutex m_mutex;                //»¥³âËø	
#endif
	uint32_t m_capacity;
	uint32_t m_buffsize;
	uint32_t m_read;
	uint32_t m_write;
	uint32_t m_writecount;
	uint32_t m_commitcount;
};


inline bool circular_buffer::is_init()
{
	return static_cast<bool>(m_buffer);
}

inline void circular_buffer::uninit()
{
	if (m_buffer)
	{
		m_buffer.reset();
	}

	m_buffsize = 0;
	m_read = 0;
	m_write = 0;
}

inline uint32_t circular_buffer::get_write_count()
{
	return m_writecount;
}

inline uint32_t circular_buffer::get_commit_count()
{
	return m_commitcount;
}


#endif
