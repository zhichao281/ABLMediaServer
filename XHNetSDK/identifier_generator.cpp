#ifdef USE_BOOST
#include <boost/unordered_set.hpp>
#include "identifier_generator.h"
#include "auto_lock.h"

boost::unordered_set<NETHANDLE> g_idset;
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
auto_lock::al_mutex g_idmtx;
#else
auto_lock::al_spin g_idmtx;
#endif

NETHANDLE generate_identifier()
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_idmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_idmtx);
#endif

	static NETHANDLE s_id = 1;
	boost::unordered_set<NETHANDLE>::iterator it;

	for (;;)
	{
		it = g_idset.find(s_id);
		if ((g_idset.end() == it) && (0 != s_id))
		{
			std::pair<boost::unordered_set<NETHANDLE>::iterator, bool> ret = g_idset.insert(s_id);
			if (ret.second)
			{
				break;	//useful
			}
		}
		else
		{
			++s_id;
		}
	}

	return s_id++;
}

void  recycle_identifier(NETHANDLE id)
{
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(g_idmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(g_idmtx);
#endif

	boost::unordered_set<NETHANDLE>::iterator it = g_idset.find(id);
	if (g_idset.end() != it)
	{
		g_idset.erase(it);
	}
}
#else
#include "identifier_generator.h"
#include <unordered_set>
#include <queue>
#include <mutex>
#include <limits>

std::unordered_set<NETHANDLE> g_idset;  // 当前已分配的 ID
std::queue<NETHANDLE> g_recycle_queue;  // 回收的 ID
std::mutex g_idmtx;

NETHANDLE generate_identifier()
{
	std::lock_guard<std::mutex> lock(g_idmtx);
	static NETHANDLE s_next_id = 1; // 从 1 开始

	NETHANDLE id = 0;

	// 1. 优先使用回收的 ID
	if (!g_recycle_queue.empty())
	{
		id = g_recycle_queue.front();
		g_recycle_queue.pop();
		g_idset.insert(id);
		return id;
	}

	// 2. 如果回收列表空了，就分配新的 ID
	if (s_next_id == 0) // 跳过 0
		++s_next_id;

	if (g_idset.size() >= std::numeric_limits<NETHANDLE>::max())
		throw std::runtime_error("No available identifiers!");

	id = s_next_id++;
	if (s_next_id == std::numeric_limits<NETHANDLE>::max())
		s_next_id = 1; // 回绕

	g_idset.insert(id);
	return id;
}

void recycle_identifier(NETHANDLE id)
{
	std::lock_guard<std::mutex> lock(g_idmtx);

	auto it = g_idset.find(id);
	if (it != g_idset.end())
	{
		g_idset.erase(it);
		g_recycle_queue.push(id); // 回收到可用列表
	}
}
#endif
