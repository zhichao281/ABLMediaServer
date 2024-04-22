//
// Created by Piasy on 04/11/2017.
//

#pragma once

#include <memory>

#include "avx_helper.h"

#include "FFmpegAVFrameSWS.h"
#include "spdloghead.h"
#define DUMP_AUDIO 0

#define DEFAULT_AUDIO_CODEC_ID		AV_CODEC_ID_AAC
#define DEFALUT_AUDIO_CHANNELS		2
#define DEFAULT_AUDIO_FORMAT		AV_SAMPLE_FMT_FLTP
#define DEFAULT_AUDIO_SAMPLE_RATE	48000
#define DEFALUT_AUDIO_BIT_RATE		192000
#define DEFALUT_AUDIO_FRMAE_SIZE	1152

//    resampler_.reset(new AudioResampler(input_format_, input_sample_rate_, input_channel_num_,
//kOutputSampleFormat, output_sample_rate_,
//output_channel_num_));
//return resampler_->Resample(input_buffer_, consumed, buffer);
namespace audio_mixer {
/// <summary>
/// 
/// </summary>
class AudioResampler :public AudioResamplerAPI
{
public:
	AudioResampler();

	int ResampleInit(
            AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channel_num,
            AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channel_num
    );


    virtual ~AudioResampler() {
		SPDLOG_LOGGER_INFO(spdlogptr, "delete AudioResampler  start={} ", static_cast<void*>(this));
		if (m_pcm_fifo)
		{
			av_audio_fifo_free(m_pcm_fifo);
			m_pcm_fifo = nullptr;
		}
		if (dst_data)
			av_freep(&dst_data[0]);
		av_freep(&dst_data);

		// 不再手动释放 SwrContext，std::unique_ptr 会自动处理

#if DUMP_AUDIO
		if (decode_file) {
			fclose(decode_file);
			decode_file = nullptr;
		}
#endif
		SPDLOG_LOGGER_INFO(spdlogptr, "delete AudioResampler  start={} ", static_cast<void*>(this));
	}	
	//virtual int ResampleAudio(uint8_t** input_buffer, int input_size, uint8_t** output_buffer, int* oLength);

	virtual int32_t Resample(uint8_t** out, int32_t* output_samples, const uint8_t** in, int in_samples, bool bAddfifo = false);

	int32_t Resample(void** input_buffer, int32_t input_size, void** output_buffer);

	virtual int FillingPCM(uint8_t* input_buffer[AV_NUM_DATA_POINTERS], int input_size);

	virtual int ResampleAudio(uint8_t** input_buffer, int input_size);

	virtual int GetOneFrame(uint8_t* output_buffer[AV_NUM_DATA_POINTERS], int* oLength);

	virtual void SetOutFramesize(int frame_size) { 
		if (frame_size>0)
		{
			m_out_frame_size = frame_size;
		}
		};
	virtual uint8_t** GetOneFrame()
	{	
		return dst_data;
	}

private:
	int AddToFifo(void** data, int size);

	bool CreatePcmFifo();

	int ResampleAndFifo(uint8_t* iBuffer, int iLength);
private:

    std::unique_ptr<SwrContext, SwrContextDeleter> m_context;
    AVSampleFormat m_input_format;
    int32_t m_input_sample_rate;
    int32_t m_input_channels;
    AVSampleFormat m_output_format;
    int32_t m_output_sample_rate;
    int32_t m_output_channels;

	const int ALIGNMENT = 1;
	const int NO_FLAGS = 0;
	uint8_t** dst_data = NULL;
	int dst_linesize;
	int m_src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;

	int m_out_frame_size;
	bool m_convert;  //是否启动转码
	AVAudioFifo* m_pcm_fifo; 
	int m_current_max_fifo_size;// 默认缓存1s的数据大小，最大不超过5s		
#ifdef DUMP_AUDIO
	FILE* decode_file;
	char  szFileName[256];
	int   nWritePcmCount;
#endif
};

}
//
//class AudioResampler :public AudioResamplerAPI
//{
//public:
//
//	AudioResampler(
//		AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channel_num,
//		AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channel_num
//	);
//
//	// 析构函数
//	virtual ~AudioResampler() {
//		if (dst_data)
//			av_freep(&dst_data[0]);
//		av_freep(&dst_data);
//		// 不再手动释放 SwrContext，std::unique_ptr 会自动处理
//	}
//
//
//	virtual int32_t Resample(const uint8_t** src_data, int in_count, uint8_t** output_buffer);
//
//
//	int getlinesize() {
//		return dst_linesize;
//	}
//private:
//
//	std::unique_ptr<SwrContext, audio_mixer::SwrContextDeleter> m_context;
//	AVSampleFormat m_input_format;
//	int32_t m_input_sample_rate;
//	int32_t m_input_channel_num;
//	AVSampleFormat m_output_format;
//	int32_t m_output_sample_rate;
//	int32_t m_output_channel_num;
//
//	const int ALIGNMENT = 1;
//	const int NO_FLAGS = 0;
//	uint8_t** dst_data = NULL;
//	int dst_linesize;
//	int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
//#ifdef DUMP_AUDIO
//	FILE* fWritePcm;
//	char  szFileName[256];
//	int   nWritePcmCount;
//#endif
//};
