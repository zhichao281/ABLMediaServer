#ifdef USE_BOOST
#ifndef _AUTO_LOCK_H_
#define _AUTO_LOCK_H_ 

#ifdef _WIN32

#include <Windows.h>

#if (__cplusplus < 201103L)
#include <boost/thread/mutex.hpp>
#else
#include <mutex>
#endif


namespace auto_lock
{
	class al_spin
	{
	public:
		al_spin()
		{
			InitializeCriticalSectionAndSpinCount(&m_spin, 4000);
		}

		~al_spin()
		{
			DeleteCriticalSection(&m_spin);
		}

		bool try_lock()
		{
			return (TRUE == TryEnterCriticalSection(&m_spin));
		}

		void lock()
		{
			EnterCriticalSection(&m_spin);
		}

		void unlock()
		{
			LeaveCriticalSection(&m_spin);
		}

	private:
		al_spin(const al_spin&)/* = delete*/;
		al_spin& operator=(const al_spin&)/* = delete*/;

	private:
		CRITICAL_SECTION m_spin;
	};

#if (__cplusplus < 201103L)
	typedef boost::mutex al_mutex;
#else
	typedef std::mutex al_mutex;
#endif


}

#else

#include <stdexcept>
#include <pthread.h>

namespace auto_lock
{
	class al_spin
	{
	public:
		al_spin()
		{
			if (0 != pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE))
			{
				throw std::runtime_error("pthread_spin_init error");
			}
		}

		~al_spin()
		{
			pthread_spin_destroy(&m_spin);
		}

		bool try_lock()
		{
			return (0 == pthread_spin_trylock(&m_spin));
		}

		void lock()
		{
			if (0 != pthread_spin_lock(&m_spin))
			{
				throw std::runtime_error("pthread_spin_lock error");
			}
		}

		void unlock()
		{
			pthread_spin_unlock(&m_spin);
		}

	private:
		al_spin(const al_spin&)/* = delete*/;
		al_spin& operator=(const al_spin&)/* = delete*/;

	private:
		pthread_spinlock_t m_spin;
	};

	class al_mutex
	{
	public:
		al_mutex()
		{

			if (0 != pthread_mutex_init(&m_mtx, NULL))
			{
				throw std::runtime_error("pthread_mutex_init error");
			}

		}

		~al_mutex()
		{
			pthread_mutex_destroy(&m_mtx);
		}

		bool try_lock()
		{
			return (0 == pthread_mutex_trylock(&m_mtx));
		}

		void lock()
		{
			if (0 != pthread_mutex_lock(&m_mtx))
			{
				throw std::runtime_error("pthread_mutex_lock error");
			}
		}

		void unlock()
		{
			pthread_mutex_unlock(&m_mtx);
		}

	private:
		al_mutex(const al_mutex&);
		al_mutex& operator=(const al_mutex&);

	private:
		pthread_mutex_t m_mtx;
	};
}

#endif

namespace auto_lock
{
	template<typename T> class al_lock
	{
	public:
		al_lock(T& m)
			: m_mtx(m)
		{
			m_mtx.lock();
		}

		~al_lock()
		{
			m_mtx.unlock();
		}

	private:
		al_lock(const al_lock&)/* = delete*/;
		al_lock& operator=(const al_lock&)/* = delete*/;

	private:
		T& m_mtx;
	};
}

#endif

#else

#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <mutex>
namespace auto_lock
{
	class al_spin
	{
	public:
		al_spin()
		{
			InitializeCriticalSectionAndSpinCount(&m_spin, 4000);
		}

		~al_spin()
		{
			DeleteCriticalSection(&m_spin);
		}

		bool try_lock()
		{
			return (TRUE == TryEnterCriticalSection(&m_spin));
		}

		void lock()
		{
			EnterCriticalSection(&m_spin);
		}

		void unlock()
		{
			LeaveCriticalSection(&m_spin);
		}

	private:
		al_spin(const al_spin&)/* = delete*/;
		al_spin& operator=(const al_spin&)/* = delete*/;

	private:
		CRITICAL_SECTION m_spin;
	};
	typedef std::mutex al_mutex;

}

#else

#include <stdexcept>
#include <pthread.h>

namespace auto_lock
{
	class al_spin
	{
	public:
		al_spin()
		{
			if (0 != pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE))
			{
				throw std::runtime_error("pthread_spin_init error");
			}
		}

		~al_spin()
		{
			pthread_spin_destroy(&m_spin);
		}

		bool try_lock()
		{
			return (0 == pthread_spin_trylock(&m_spin));
		}

		void lock()
		{
			if (0 != pthread_spin_lock(&m_spin))
			{
				throw std::runtime_error("pthread_spin_lock error");
			}
		}

		void unlock()
		{
			pthread_spin_unlock(&m_spin);
		}

	private:
		al_spin(const al_spin&)/* = delete*/;
		al_spin& operator=(const al_spin&)/* = delete*/;

	private:
		pthread_spinlock_t m_spin;
	};

	class al_mutex
	{
	public:
		al_mutex()
		{

			if (0 != pthread_mutex_init(&m_mtx, NULL))
			{
				throw std::runtime_error("pthread_mutex_init error");
			}

		}

		~al_mutex()
		{
			pthread_mutex_destroy(&m_mtx);
		}

		bool try_lock()
		{
			return (0 == pthread_mutex_trylock(&m_mtx));
		}

		void lock()
		{
			if (0 != pthread_mutex_lock(&m_mtx))
			{
				throw std::runtime_error("pthread_mutex_lock error");
			}
		}

		void unlock()
		{
			pthread_mutex_unlock(&m_mtx);
		}

	private:
		al_mutex(const al_mutex&);
		al_mutex& operator=(const al_mutex&);

	private:
		pthread_mutex_t m_mtx;
	};
}

#endif

namespace auto_lock
{
	template<typename T> class al_lock
	{
	public:
		al_lock(T& m)
			: m_mtx(m)
		{
			m_mtx.lock();
		}

		~al_lock()
		{
			m_mtx.unlock();
		}

	private:
		al_lock(const al_lock&)/* = delete*/;
		al_lock& operator=(const al_lock&)/* = delete*/;

	private:
		T& m_mtx;
	};
}


#endif

