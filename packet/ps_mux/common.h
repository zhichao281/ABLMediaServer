#ifndef _PS_MUX_DEMUX_MUX_COMMON_H_
#define _PS_MUX_DEMUX_MUX_COMMON_H_

#include <stdint.h>

/***************H264***************/
enum e_h264_nalu_type
{
	e_h264_nal_unknow = 0,

	e_h264_nal_slice,

	e_h264_nal_dpa,

	e_h264_nal_dpb,

	e_h264_nal_dpc,

	e_h264_nal_idr,

	e_h264_nal_sei,

	e_h264_nal_sps,

	e_h264_nal_pps,

	e_h264_nal_aud,

	e_h264_nal_eoseq,

	e_h264_nal_eostream,

	e_h264_nal_fill,
};

const uint8_t H264_NALU_START_CODE[] = { 0x00, 0x00, 0x01 };
const uint8_t H264_SLICE_START_CODE[] = { 0X00, 0X00, 0X00, 0X01 };



/***************H265***************/
enum e_h265_nalu_type
{
	e_h265_nal_trail_n = 0,

	e_h265_nal_trail_r = 1,

	e_h265_nal_tsa_n = 2,

	e_h265_nal_tsa_r = 3,

	e_h265_nal_stsa_n = 4,

	e_h265_nal_stsa_r = 5,

	e_h265_nal_radl_n = 6,

	e_h265_nal_radl_r = 7,

	e_h265_nal_rasl_n = 8,

	e_h265_nal_rasl_r = 9,

	e_h265_nal_bla_w_lp = 16,

	e_h265_nal_bla_w_radl = 17,

	e_h265_nal_bla_n_lp = 18,

	e_h265_nal_idr_w_radl = 19,

	e_h265_nal_idr_n_lp = 20,

	e_h265_nal_cra_nut = 21,

	e_h265_nal_vps = 32,

	e_h265_nal_sps = 33,

	e_h265_nal_pps = 34,

	e_h265_nal_aud = 35,

	e_h265_nal_eos_nut = 36,

	e_h265_nal_eob_nut = 37,

	e_h265_nal_fd_nut = 38,

	e_h265_nal_sei_prefix = 39,

	e_h265_nal_sei_suffix = 40,
};

const uint8_t H265_NALU_START_CODE[] = { 0x00, 0x00, 0x01 };
const uint8_t H265_SLICE_START_CODE[] = { 0X00, 0X00, 0X00, 0X01 };



/**************SVAC************/
enum e_svac_nalu_type
{
	e_svac_nal_unknown = 0,

	e_svac_nal_pb,

	e_svac_nal_idr,

	e_svac_nal_pb_enhancement,

	e_svac_nal_idr_enhancement,

	e_svac_nal_surveillance_extension,

	e_svac_nal_sei,

	e_svac_nal_sps,

	e_svac_nal_pps,

	e_svac_nal_sec,

	e_svac_nal_end_seq,

	e_svac_nal_end_stream,

	e_svac_nal_fill,
};

const uint8_t SVAC_NALU_START_CODE[] = { 0x00, 0x00, 0x01 };
const uint8_t SVAC_SLICE_START_CODE[] = { 0X00, 0X00, 0X00, 0X01 };



/***************MPEG-4***************/
enum e_mpeg4_object_type
{
	e_mp4v_video_object_min_start = 0x00,

	e_mp4v_video_object_max_start = 0x1f,

	e_mp4v_video_object_layer_min_start = 0x20,

	e_mp4v_video_object_layer_max_start = 0x2f,

	e_mp4v_visual_object_sequence_start = 0xb0,

	e_mp4v_visual_object_sequence_end = 0xb1,

	e_mp4v_user_data_start = 0xb2,

	e_mp4v_group_of_vop_start = 0xb3,

	e_mp4v_video_session_error = 0xb4,

	e_mp4v_visual_object_start = 0xb5,

	e_mp4v_vop_start = 0xb6,
};

const uint8_t MPEG4_START_CODE[] = { 0x00, 0x00, 0x01 };




/***************MPEG-2***************/
enum
{
	e_mp2v_picture_start = 0x00,

	e_mp2v_slice_min_start = 0x01,

	e_mp2v_slice_max_start = 0xaf,

	e_mp2v_user_data_start = 0xb2,

	e_mp2v_sequence_header = 0xb3,

	e_mp2v_error = 0xb4,

	e_mp2v_extension_start = 0xb5,

	e_mp2v_sequence_end = 0xb7,

	e_mp2v_group_start = 0xb8,
};

const uint8_t MPEG2_START_CODE[] = { 0x00, 0x00, 0x01 };




uint32_t generate_identifier();

void recycle_identifier(uint32_t id);

int32_t get_mediatype(int32_t st);

int32_t get_frametype(int32_t st, uint8_t* data, uint32_t datasize);

/*
int32_t mpeg1frametype(uint8_t* data, uint32_t datasize);

int32_t mpeg2frametype(uint8_t* data, uint32_t datasize);

int32_t mjpegframetype(uint8_t* data, uint32_t datasize);

int32_t mpeg4frametype(uint8_t* data, uint32_t datasize);

int32_t h264frametype(uint8_t* data, uint32_t datasize);

int32_t h265frametype(uint8_t* data, uint32_t datasize);

int32_t svacframetype(uint8_t* data, uint32_t datasize);
*/

#endif 