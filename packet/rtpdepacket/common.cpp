#include <unordered_set>
#include <mutex>

#include "common.h"

std::unordered_set<uint32_t> g_identifier_setRtpDepacket;

std::mutex g_identifier_mutex;


uint32_t generate_identifier()
{

	std::lock_guard<std::mutex> lg(g_identifier_mutex);


	static uint32_t s_id = 1;
	std::unordered_set<uint32_t>::iterator it;

	for (;;)
	{
		it = g_identifier_setRtpDepacket.find(s_id);
		if ((g_identifier_setRtpDepacket.end() == it) && (0 != s_id))
		{
			auto ret = g_identifier_setRtpDepacket.insert(s_id);
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
	std::lock_guard<std::mutex> lg(g_identifier_mutex);
	auto it = g_identifier_setRtpDepacket.find(id);
	if (g_identifier_setRtpDepacket.end() != it)
	{
		g_identifier_setRtpDepacket.erase(it);
	}
}