#pragma once

#include "ffmpegTypes.h"
#include "mediabase.h"


// PCM音频流格式定义，与FFMPEG中一一对应
typedef enum
{
	AUDIOMIX_FMT_NONE = -1,
	AUDIOMIX_FMT_U8,          ///< unsigned 8 bits
	AUDIOMIX_FMT_S16,         ///< signed 16 bits
	AUDIOMIX_FMT_S32,         ///< signed 32 bits
	AUDIOMIX_FMT_FLT,         ///< float
	AUDIOMIX_FMT_DBL,         ///< double
	
	AUDIOMIX_FMT_U8P,         ///< unsigned 8 bits, planar
	AUDIOMIX_FMT_S16P,        ///< signed 16 bits, planar
	AUDIOMIX_FMT_S32P,        ///< signed 32 bits, planar
	AUDIOMIX_FMT_FLTP,        ///< float, planar
	AUDIOMIX_FMT_DBLP,        ///< double, planar
	AUDIOMIX_FMT_S64,         ///< signed 64 bits
	AUDIOMIX_FMT_S64P,        ///< signed 64 bits, planar
	AUDIOMIX_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically

}AUDIOMIX_AVSampleFormat;



class AudioResamplerAPI
{
#define AV_NUM_DATA_POINTERS 8
public:
	AudioResamplerAPI() {};
	virtual ~AudioResamplerAPI() {};

	static AudioResamplerAPI * CreateAudioResampler(
		AUDIOMIX_AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channel_num,
		AUDIOMIX_AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channel_num
	);

	
	virtual int32_t Resample(uint8_t** out, int32_t* output_samples, const uint8_t** in, int in_count, bool bAddfifo = false) = 0;
	
	
	//virtual int ResampleAudio(uint8_t** input_buffer, int input_size, uint8_t** output_buffer, int* oLength)=0;
	virtual int ResampleAudio(uint8_t** input_buffer, int input_size)=0;

	virtual uint8_t** GetOneFrame()=0;

	virtual int FillingPCM(uint8_t* input_buffer[AV_NUM_DATA_POINTERS], int input_size) =0;

	virtual int GetOneFrame(uint8_t* output_buffer[AV_NUM_DATA_POINTERS], int* oLength)=0;

	virtual void SetOutFramesize(int frame_size)=0;
};