#pragma  once
#ifdef USE_BOOST
#include <stdexcept>
#include <boost/unordered_set.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include "identifier_generator.h"

template<typename T>
class object_pool
{
public:
	object_pool(uint32_t capacity);
	~object_pool();

	NETHANDLE get_id();
	T* malloc(boost::asio::io_context& ioc,
		NETHANDLE srvid,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	void free(const T* obj);
	bool full();
	bool empty();

private:
	object_pool(const object_pool&);
	object_pool& operator=(const object_pool&);

private:
	uint8_t* m_memory;
	boost::mutex m_mutex;
	boost::unordered_set<uint32_t> m_indexset;
	const uint32_t m_capacity;
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	const NETHANDLE m_id;
#endif
};


template<typename T> object_pool<T>::object_pool(uint32_t capacity)
	: m_capacity(capacity)
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	, m_id(generate_identifier())
#endif
{
	if (m_capacity > 0)
	{
		try
		{
			m_memory = new uint8_t[sizeof(T) * m_capacity];
		}
		catch (const std::bad_alloc& /*e*/)
		{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
			recycle_identifier(m_id);
#endif
			throw;
		}
		catch (...)
		{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
			recycle_identifier(m_id);
#endif
			throw;
		}
	}
	else
	{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
		recycle_identifier(m_id);
#endif
		throw std::runtime_error("object number is invalid");
	}
}

template<typename T> object_pool<T>::~object_pool()
{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	recycle_identifier(m_id);
#endif
	delete[] m_memory;
}

#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
template<typename T> NETHANDLE object_pool<T>::get_id()
{
	return m_id;
}
#else
template<typename T> NETHANDLE object_pool<T>::get_id()
{
	return reinterpret_cast<NETHANDLE>(this);
}
#endif

template<typename T> T* object_pool<T>::malloc(boost::asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
{
	boost::lock_guard<boost::mutex> lg(m_mutex);

	T* obj = NULL;

	if (m_capacity > static_cast<uint32_t>(m_indexset.size()))
	{
		for (uint32_t index = 1; index <= m_capacity; ++index)
		{
			if (m_indexset.end() == m_indexset.find(index))
			{
				std::pair<boost::unordered_set<uint32_t>::iterator, bool> ret = m_indexset.insert(index);
				if (ret.second)
				{
					uint8_t* mem = m_memory + sizeof(T) * (index - 1);
					obj = new(mem)T(ioc, srvid, fnread, fnclose, autoread);
					obj->set_pool_id(get_id());
					obj->set_pool_index(static_cast<NETHANDLE>(index));

					break;
				}
			}
		}
	}

	return obj;
}

template<typename T> void object_pool<T>::free(const T* obj)
{
	boost::lock_guard<boost::mutex> lg(m_mutex);

	if (obj && (get_id() == obj->get_pool_id()))
	{
		boost::unordered_set<uint32_t>::iterator it = m_indexset.find(static_cast<uint32_t>(obj->get_pool_index()));
		if (m_indexset.end() != it)
		{
			m_indexset.erase(it);
		}
	}
}

template<typename T> bool object_pool<T>::full()
{
	boost::lock_guard<boost::mutex> lg(m_mutex);

	return (m_capacity == static_cast<uint32_t>(m_indexset.size()));
}

template<typename T> bool object_pool<T>::empty()
{
	boost::lock_guard<boost::mutex> lg(m_mutex);

	return m_indexset.empty();
}



#else

#include <stdexcept>
#include <unordered_set>
#include "identifier_generator.h"

template<typename T>
class object_pool
{
public:
	object_pool(uint32_t capacity);
	~object_pool();

	NETHANDLE get_id();
	T* malloc(asio::io_context& ioc,
		NETHANDLE srvid,
		read_callback fnread,
		close_callback fnclose,
		bool autoread);
	void free(const T* obj);
	bool full();
	bool empty();

private:
	object_pool(const object_pool&);
	object_pool& operator=(const object_pool&);

private:
	uint8_t* m_memory;
	std::mutex m_mutex;                //»¥³âËø	
	std::unordered_set<uint32_t> m_indexset;
	const uint32_t m_capacity;
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	const NETHANDLE m_id;
#endif
};


template<typename T> object_pool<T>::object_pool(uint32_t capacity)
	: m_capacity(capacity)
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	, m_id(generate_identifier())
#endif
{
	if (m_capacity > 0)
	{
		try
		{
			m_memory = new uint8_t[sizeof(T) * m_capacity];
		}
		catch (const std::bad_alloc& /*e*/)
		{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
			recycle_identifier(m_id);
#endif
			throw;
		}
		catch (...)
		{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
			recycle_identifier(m_id);
#endif
			throw;
		}
	}
	else
	{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
		recycle_identifier(m_id);
#endif
		throw std::runtime_error("object number is invalid");
	}
}

template<typename T> object_pool<T>::~object_pool()
{
#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
	recycle_identifier(m_id);
#endif
	delete[] m_memory;
}

#ifndef LIBNET_USE_THIS_POINTER_AS_HANDLE
template<typename T> NETHANDLE object_pool<T>::get_id()
{
	return m_id;
}
#else
template<typename T> NETHANDLE object_pool<T>::get_id()
{
	return reinterpret_cast<NETHANDLE>(this);
}
#endif

template<typename T> T* object_pool<T>::malloc(asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
{
	std::unique_lock<std::mutex> _lock(m_mutex);
	T* obj = NULL;

	if (m_capacity > static_cast<uint32_t>(m_indexset.size()))
	{
		for (uint32_t index = 1; index <= m_capacity; ++index)
		{
			if (m_indexset.end() == m_indexset.find(index))
			{
				auto ret = m_indexset.insert(index);
				if (ret.second)
				{
					uint8_t* mem = m_memory + sizeof(T) * (index - 1);
					obj = new(mem)T(ioc, srvid, fnread, fnclose, autoread);
					obj->set_pool_id(get_id());
					obj->set_pool_index(static_cast<NETHANDLE>(index));

					break;
				}
			}
		}
	}

	return obj;
}

template<typename T> void object_pool<T>::free(const T* obj)
{
	std::unique_lock<std::mutex> _lock(m_mutex);

	if (obj && (get_id() == obj->get_pool_id()))
	{
		auto it = m_indexset.find(static_cast<uint32_t>(obj->get_pool_index()));
		if (m_indexset.end() != it)
		{
			m_indexset.erase(it);
		}
	}
}

template<typename T> bool object_pool<T>::full()
{
	std::unique_lock<std::mutex> _lock(m_mutex);

	return (m_capacity == static_cast<uint32_t>(m_indexset.size()));
}

template<typename T> bool object_pool<T>::empty()
{
	std::unique_lock<std::mutex> _lock(m_mutex);

	return m_indexset.empty();
}



#endif