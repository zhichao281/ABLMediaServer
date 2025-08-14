#include <unordered_set>
#include <memory>
#include <mutex>
#include "common.h"
#include "ps_mux.h"

std::unordered_set<uint32_t> g_identifier_set;


std::mutex g_identifier_mutex;


uint32_t generate_identifier()
{
	std::lock_guard<std::mutex> lg(g_identifier_mutex);

	static uint32_t s_id = 1;
	std::unordered_set<uint32_t>::iterator it;

	for (;;)
	{
		it = g_identifier_set.find(s_id);
		if ((g_identifier_set.end() == it) && (0 != s_id))
		{
			auto ret = g_identifier_set.insert(s_id);
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

	auto it = g_identifier_set.find(id);
	if (g_identifier_set.end() != it)
	{
		g_identifier_set.erase(it);
	}
}

int32_t get_mediatype(int32_t st)
{
	int32_t mt = e_psmux_mt_unknown;

	switch (st)
	{

	case e_psmux_st_mpeg1v:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_mpeg2v:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_mpeg1a:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_mpeg2a:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_mjpeg:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_aac:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_mpeg4:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_aac_latm:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_h264:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_h265:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_svacv:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_pcm:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_pcmlaw:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_g711a:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_g711u:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_g7221:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_g7231:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_g729:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_svaca:
	{
		mt = e_psmux_mt_audio;

	}break;

	case e_psmux_st_hkp:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_hwp:
	{
		mt = e_psmux_mt_video;

	}break;

	case e_psmux_st_dhp:
	{
		mt = e_psmux_mt_video;

	}break;


	default:
		break;
	}

	return mt;
}

int32_t mpeg1frametype(uint8_t* data, uint32_t datasize);

int32_t mpeg2frametype(uint8_t* data, uint32_t datasize);

int32_t mjpegframetype(uint8_t* data, uint32_t datasize);

int32_t mpeg4frametype(uint8_t* data, uint32_t datasize);

int32_t h264frametype(uint8_t* data, uint32_t datasize);

int32_t h265frametype(uint8_t* data, uint32_t datasize);

int32_t svacframetype(uint8_t* data, uint32_t datasize);

int32_t get_frametype(int32_t st, uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;

	if (!data || (0 == datasize))
	{
		return ft;
	}

	switch (st)
	{
	case e_psmux_st_mpeg1v:
	{
		ft = mpeg1frametype(data, datasize);

	}break;

	case e_psmux_st_mpeg2v:
	{
		ft = mpeg2frametype(data, datasize);

	}break;

	case e_psmux_st_mjpeg:
	{
		ft = mjpegframetype(data, datasize);

	}break;

	case e_psmux_st_mpeg4:
	{
		ft = mpeg4frametype(data, datasize);

	}break;

	case e_psmux_st_h264:
	{
		ft = h264frametype(data, datasize);

	}break;

	case e_psmux_st_h265:
	{
		ft = h265frametype(data, datasize);

	}break;

	case e_psmux_st_svacv:
	{
		ft = svacframetype(data, datasize);

	}break;

	default:
		break;

	}

	
	return ft;
}

int32_t mpeg1frametype(uint8_t* data, uint32_t datasize)
{
	return mpeg2frametype(data, datasize);
}

int32_t mpeg2frametype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;
	uint32_t pos = 0;

	if (datasize <= (sizeof(MPEG2_START_CODE) + 2))
	{
		return ft;
	}

	for (; pos < (datasize - sizeof(MPEG2_START_CODE) - 2);)
	{
		if (0 == memcmp(data + pos, MPEG2_START_CODE, sizeof(MPEG2_START_CODE)))
		{
			switch (data[pos + sizeof(MPEG2_START_CODE)])
			{
			case e_mp2v_picture_start:
			{
				//picture_header(32)->temporal_reference(10)->picture_coding_type(3)
				//1-i frame 2-p frame 3-b frame
				ft = (data[pos + sizeof(MPEG2_START_CODE) + 2] >> 3) & 0x07;

			}break;

			default:
			{
				pos += (sizeof(MPEG2_START_CODE) + 2);

			}break;

			}

			if (e_psmux_ft_unknown != ft)
			{
				break;
			}
		}
		else
		{
			if (0 == pos)
			{
				break;
			}
			else
			{
				++pos;
				continue;
			}
		}
	}

	return ft;
}

int32_t mjpegframetype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_i;

	return ft;
}

int32_t mpeg4frametype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;
	uint32_t pos = 0;

	if (datasize <= (sizeof(MPEG4_START_CODE) + 1))
	{
		return ft;
	}

	for (; pos < (datasize - sizeof(MPEG4_START_CODE) - 1);)
	{
		if (0 == memcmp(data + pos, MPEG4_START_CODE, sizeof(MPEG4_START_CODE)))
		{
			switch (data[pos + sizeof(MPEG4_START_CODE)])
			{
			case e_mp4v_vop_start:
			{
				//0-i frame  1-p frame  2-b frame 3-s frame
				ft = (data[pos + sizeof(MPEG4_START_CODE) + 1] >> 6) & 0x03 + 1;

			}break;

			default:
			{
				pos += (sizeof(MPEG4_START_CODE) + 2);

			}break;

			}

			if (e_psmux_ft_unknown != ft)
			{
				break;
			}
		}
		else
		{
			if (0 == pos)
			{
				break;
			}
			else
			{
				++pos;
				continue;
			}
		}
	}

	return ft;
}


int32_t h264frametype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;
	uint32_t pos = 0;
	uint32_t sclen = 0;
	int32_t nalutype = e_h264_nal_unknow;

	if (datasize <= (sizeof(H264_SLICE_START_CODE) + 1) )
	{
		return ft;
	}

	for (; pos < (datasize - sizeof(H264_SLICE_START_CODE ) - 1 ); )
	{
		if (0 == memcmp(data + pos, H264_NALU_START_CODE, sizeof(H264_NALU_START_CODE)))
		{
			sclen = sizeof(H264_NALU_START_CODE);
		}
		else if (0 == memcmp(data + pos, H264_SLICE_START_CODE, sizeof(H264_SLICE_START_CODE)))
		{
			sclen = sizeof(H264_SLICE_START_CODE);
		}
		else
		{
			if (0 == pos)
			{
				break;
			}
			else
			{
				++pos;
				continue;
			}
		}

		nalutype = data[pos + sclen] & 0x1f;

		switch (nalutype)
		{ 
		case e_h264_nal_slice:
		{
			ft = e_psmux_ft_p;

		}break;

		case e_h264_nal_dpa:
		case e_h264_nal_idr:
		case e_h264_nal_sps:
		{
			ft = e_psmux_ft_i;

		}break;

		default:
		{
			pos += sclen;

		}break;

		}

		if (e_psmux_ft_unknown != ft)
		{
			break;
		}
		else
		{
			++pos;
		}
		
	}

	return ft;

}

int32_t h265frametype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;
	uint32_t pos = 0;
	uint32_t sclen = 0;
	int32_t nalutype = e_h265_nal_trail_n;

	if (datasize <= (sizeof(H265_SLICE_START_CODE) + 2))
	{
		return ft;
	}

	for (; pos < (datasize - sizeof(H265_SLICE_START_CODE) - 2);)
	{
		if (0 == memcmp(data + pos, H265_NALU_START_CODE, sizeof(H265_NALU_START_CODE)))
		{
			sclen = sizeof(H265_NALU_START_CODE);
		}
		else if (0 == memcmp(data + pos, H265_SLICE_START_CODE, sizeof(H265_SLICE_START_CODE)))
		{
			sclen = sizeof(H265_SLICE_START_CODE);
		}
		else
		{
			if (0 == pos)
			{
				break;
			}
			else
			{
				++pos;
				continue;
			}
		}

		nalutype = (data[pos + sclen] >> 1) & 0x3f;

		switch (nalutype)
		{
		case e_h265_nal_trail_r:
		case e_h265_nal_tsa_n:
		case e_h265_nal_tsa_r:
		case e_h265_nal_stsa_n:
		case e_h265_nal_stsa_r:
		case e_h265_nal_radl_n:
		case e_h265_nal_radl_r:
		case e_h265_nal_rasl_n:
		case e_h265_nal_rasl_r:
		{
			ft = e_psmux_ft_p;

		}break;

		case e_h265_nal_bla_w_lp:
		case e_h265_nal_bla_w_radl:
		case e_h265_nal_bla_n_lp:
		case e_h265_nal_idr_w_radl:
		case e_h265_nal_idr_n_lp:
		case e_h265_nal_cra_nut:
		case e_h265_nal_vps:
		{
			ft = e_psmux_ft_i;

		}break;

		default:
		{
			pos += sclen;

		}break;

		}

		if (e_psmux_ft_unknown != ft)
		{
			break;
		}
		else
		{
			pos += 2;
		}

	}

	return ft;
}

int32_t svacframetype(uint8_t* data, uint32_t datasize)
{
	int32_t ft = e_psmux_ft_unknown;
	uint32_t pos = 0;
	uint32_t sclen = 0;
	int32_t nalutype = e_svac_nal_unknown;

	if (datasize <= (sizeof(SVAC_SLICE_START_CODE) + 1))
	{
		return ft;
	}

	for (; pos < (datasize - sizeof(SVAC_SLICE_START_CODE) - 1);)
	{
		if (0 == memcmp(data + pos, SVAC_NALU_START_CODE, sizeof(SVAC_NALU_START_CODE)))
		{
			sclen = sizeof(SVAC_NALU_START_CODE);
		}
		else if (0 == memcmp(data + pos, SVAC_SLICE_START_CODE, sizeof(SVAC_SLICE_START_CODE)))
		{
			sclen = sizeof(SVAC_SLICE_START_CODE);
		}
		else
		{
			if (0 == pos)
			{
				break;
			}
			else
			{
				++pos;
				continue;
			}
			
		}

		nalutype = (data[pos + sclen] >> 2) & 0x0f;

		switch (nalutype)
		{
		case e_svac_nal_pb:
		{
			ft = e_psmux_ft_p;

		}break;

		case e_svac_nal_idr:
		case e_svac_nal_sps:
		{
			ft = e_psmux_ft_i;

		}break;

		default:
		{
			pos += sclen;

		}break;

		}

		if (e_psmux_ft_unknown != ft)
		{
			break;
		}
		else
		{
			++pos;
		}

	}

	return ft;
}
