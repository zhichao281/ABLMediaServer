#include <unordered_set>
#include <memory>
#include <mutex>
#include "common.h"

std::unordered_set<uint32_t> g_identifier_set_psDemux;


std::mutex g_identifier_mutex_psDemux;



uint32_t generate_identifier()
{
	std::lock_guard<std::mutex> lg(g_identifier_mutex_psDemux);
	static uint32_t s_id = 1;
	for (;;)
	{
		auto it = g_identifier_set_psDemux.find(s_id);
		if ((g_identifier_set_psDemux.end() == it) && (0 != s_id))
		{
			auto ret = g_identifier_set_psDemux.insert(s_id);
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

void recycle_identifier(uint32_t id)
{
	std::lock_guard<std::mutex> lg(g_identifier_mutex_psDemux);
	auto it = g_identifier_set_psDemux.find(id);
	if (g_identifier_set_psDemux.end() != it)
	{
		g_identifier_set_psDemux.erase(it);
	}
}