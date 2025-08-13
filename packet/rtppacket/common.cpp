#include <unordered_set>
#include <mutex>
#include <memory>
#include <cstring>
#include "common.h"
#include "rtp_packet.h"

std::unordered_set<uint32_t> g_identifier_set_rtppacket;


std::mutex g_identifier_mutex;


uint32_t generate_identifier_rtppacket()
{
	std::lock_guard<std::mutex> lg(g_identifier_mutex);

	static uint32_t s_id = 1;

	for (;;)
	{
		auto it = g_identifier_set_rtppacket.find(s_id);
		if ((g_identifier_set_rtppacket.end() == it) && (0 != s_id))
		{
			auto ret = g_identifier_set_rtppacket.insert(s_id);
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

void recycle_identifier_rtppacket(uint32_t id)
{

	std::lock_guard<std::mutex> lg(g_identifier_mutex);

	auto it = g_identifier_set_rtppacket.find(id);
	if (g_identifier_set_rtppacket.end() != it)
	{
		g_identifier_set_rtppacket.erase(it);
	}
}

int32_t get_mediatype(int32_t st)
{
	int32_t mt = e_rtppkt_mt_unknown;

	switch (st)
	{

	case e_rtppkt_st_mpeg1v:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_mpeg2v:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_mpeg1a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mpeg2a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mjpeg:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_aac:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_mpeg4:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_aac_latm:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_h264:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_h265:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_svacv:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_pcm:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_pcmlaw:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g711a:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g711u:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g7221:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g7231:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_g729:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_svaca:
	{
		mt = e_rtppkt_mt_audio;

	}break;

	case e_rtppkt_st_hkp:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_hwp:
	{
		mt = e_rtppkt_mt_video;

	}break;

	case e_rtppkt_st_dhp:
	{
		mt = e_rtppkt_mt_video;

	}break;


	default:
		break;
	}

	return mt;
}