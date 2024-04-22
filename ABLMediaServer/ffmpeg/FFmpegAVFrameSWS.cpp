#include "FFmpegAVFrameSWS.h"


#include "audio_resampler.h"
#include "spdloghead.h"

AudioResamplerAPI* AudioResamplerAPI::CreateAudioResampler(AUDIOMIX_AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channel_num, AUDIOMIX_AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channel_num)
{
	auto pitem = new audio_mixer::AudioResampler();
	pitem->ResampleInit((AVSampleFormat)input_format, input_sample_rate, input_channel_num, (AVSampleFormat)output_format, output_sample_rate, output_channel_num);
	return pitem;
}
