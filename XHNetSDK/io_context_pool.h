#pragma  once 
#ifdef USE_BOOST


#include <vector>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "auto_lock.h"

class io_context_pool : public boost::noncopyable
{
public:
	io_context_pool();

	~io_context_pool();

	uint32_t get_thread_count() const { return static_cast<uint32_t>(m_threads.size()); }

	bool is_init();

	int32_t init(uint32_t iocnum, uint32_t periocthread);

	int32_t run();

	void close();

	boost::asio::io_context& get_io_context();

private:
	typedef boost::shared_ptr<boost::asio::io_context> io_context_ptr;
	typedef boost::shared_ptr<boost::asio::io_context::work> work_ptr;
	typedef boost::shared_ptr<boost::thread> thread_ptr;

	std::vector<io_context_ptr> m_iocontexts;
	std::vector<work_ptr> m_works;
	boost::thread_group m_threads;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
	uint32_t m_nextioc;
	uint32_t m_periocthread;
	bool m_isinit;
};

inline bool io_context_pool::is_init()
{
	return m_isinit;
}

#else

#include <vector>
#include <asio.hpp>
#include <memory>
#include <thread>
#include "auto_lock.h"
#include "thread_pool/thread_pool.h"

class io_context_pool : public asio::detail::noncopyable
{

public:
	io_context_pool();

	~io_context_pool();

	uint32_t get_thread_count() const { return static_cast<uint32_t>(netlib::ThreadPool::getInstance().getThreadNum()); }

	bool is_init();

	int32_t init(uint32_t iocnum, uint32_t periocthread);

	int32_t run();

	void close();

	asio::io_context& get_io_context();

private:
	typedef std::shared_ptr<asio::io_context> io_context_ptr;
	typedef std::shared_ptr<asio::io_context::work> work_ptr;
	typedef std::shared_ptr<std::thread> thread_ptr;

	std::vector<io_context_ptr> m_iocontexts;
	std::vector<work_ptr> m_works;

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_mutex m_mutex;
#else
	auto_lock::al_spin m_mutex;
#endif
	uint32_t m_nextioc;
	uint32_t m_periocthread;
	bool m_isinit;
};

inline bool io_context_pool::is_init()
{
	return m_isinit;
}


#endif

