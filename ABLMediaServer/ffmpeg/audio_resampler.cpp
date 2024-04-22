
#include "audio_resampler.h"



namespace audio_mixer {


	AudioResampler::AudioResampler():
		m_convert(false),
		m_context(swr_alloc()),
		m_pcm_fifo(nullptr)
	{
		// Create a file rotating logger with 5 MB size max and 3 rotated files

		m_out_frame_size = DEFALUT_AUDIO_FRMAE_SIZE;
	}

int AudioResampler::ResampleInit(
        AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channels,
        AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channels
)
{

	m_input_format = input_format;
	m_input_sample_rate = input_sample_rate;
	m_input_channels = input_channels;
	m_output_format = output_format;
	m_output_sample_rate = output_sample_rate;
	m_output_channels = output_channels;

	if ((m_output_channels != m_input_channels) || (m_input_sample_rate != m_output_sample_rate) || (m_output_format != m_input_format))
	{

#if DUMP_AUDIO
		decode_file = fopen("decode_file1024.pcm", "wb+");
#endif

		AVChannelLayout in_ch_layout = {}, out_ch_layout = {};
		out_ch_layout.nb_channels = m_output_channels;
		in_ch_layout.nb_channels = m_input_channels;
		av_channel_layout_default(&out_ch_layout, m_output_channels);
		av_channel_layout_default(&in_ch_layout, m_input_channels);

		int32_t error;
#if 0
		av_opt_set_chlayout(m_context.get(), "in_chlayout", &in_ch_layout, 0);
		av_opt_set_int(m_context.get(), "in_sample_rate", m_input_sample_rate, 0);
		av_opt_set_sample_fmt(m_context.get(), "in_sample_fmt", m_input_format, 0);
		av_opt_set_chlayout(m_context.get(), "out_chlayout", &out_ch_layout, 0);
		av_opt_set_int(m_context.get(), "out_sample_rate", m_output_sample_rate, 0);
		av_opt_set_sample_fmt(m_context.get(), "out_sample_fmt", m_output_format, 0);
#else
		SwrContext* contextPtr = m_context.get();
		error = swr_alloc_set_opts2(&contextPtr,
			&out_ch_layout,
			output_format,
			output_sample_rate,
			&in_ch_layout,
			input_format,
			input_sample_rate,
			0,
			0);
		if (error < 0) {
			std::cerr << "Failed to allocate SwrContext" << std::endl;
			// 处理错误，可以抛出异常或采取其他措施
		}
#endif
		error = swr_init(m_context.get());
		if (error < 0) {

			SPDLOG_LOGGER_ERROR(spdlogptr,"Failed to initialize the resampling contex\r\n");
			// 处理错误，可以抛出异常或采取其他措施
		}
		max_dst_nb_samples = dst_nb_samples =
			av_rescale_rnd(m_src_nb_samples, m_output_sample_rate, m_input_sample_rate, AV_ROUND_UP);

		auto ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, output_channels,
			output_sample_rate, output_format, 1);
		if (ret < 0) 
		{		
			SPDLOG_LOGGER_ERROR(spdlogptr,"Could not allocate source samples\r\n");
			if (dst_data)
				av_freep(&dst_data[0]);
			av_freep(&dst_data);
		}
		m_convert = true;

		if (CreatePcmFifo() == false)
		{
			//LOG(ERROR) << "CreateFifo error";
			return -1;
		}

		return ret;
	}
	

    //RTC_CHECK(error >= 0) << av_err2str(error);
}


int32_t AudioResampler::Resample(void** src_data, int32_t input_size, void** output_buffer ) {

    int32_t input_samples = input_size / m_input_channels
                            / av_get_bytes_per_sample(m_input_format);
    int32_t output_samples = static_cast<int>(
            av_rescale_rnd(input_samples, m_output_sample_rate, m_input_sample_rate, AV_ROUND_UP)
    );

    int32_t real_output_samples = swr_convert(
            m_context.get(), reinterpret_cast<uint8_t**>(output_buffer), output_samples,
            (const uint8_t**)src_data, input_samples
    );

    if (real_output_samples < 0) {
        return real_output_samples;
    }

	auto real_output_size = av_samples_get_buffer_size(nullptr, m_output_channels, real_output_samples,
		m_output_format, 1);
	return real_output_size;
}


/**
 * Initialize a temporary storage for the specified number of audio samples.
 * The conversion requires temporary storage due to the different format.
 * The number of audio samples to be allocated is specified in frame_size.
 * @param[out] converted_input_samples Array of converted samples. The
 *                                     dimensions are reference, channel
 *                                     (for multi-channel audio), sample.
 * @param      output_codec_context    Codec context of the output file
 * @param      frame_size              Number of samples to be converted in
 *                                     each round
 * @return Error code (0 if successful)
 */
static int init_converted_samples(uint8_t*** converted_input_samples,
	int nb_channels, AVSampleFormat sample_fmt,
	int frame_size)
{
	int error;

	/* Allocate as many pointers as there are audio channels.
	 * Each pointer will later point to the audio samples of the corresponding
	 * channels (although it may be NULL for interleaved formats).
	 */
	if (!(*converted_input_samples = static_cast<uint8_t**>(calloc(nb_channels,
		sizeof(**converted_input_samples))))) {
		fprintf(stderr, "Could not allocate converted input sample pointers\n");
		return AVERROR(ENOMEM);
	}

	/* Allocate memory for the samples of all channels in one consecutive
	 * block for convenience. */
	if ((error = av_samples_alloc(*converted_input_samples, NULL,
		nb_channels,
		frame_size,
		sample_fmt, 0)) < 0) {
		av_freep(&(*converted_input_samples)[0]);
		free(*converted_input_samples);
		return error;
	}
	return 0;
}


int32_t AudioResampler::Resample(uint8_t** output_buffer, int32_t * output_samples,  const uint8_t** src_data, int in_samples, bool bAddfifo)
{
	m_src_nb_samples = in_samples;
	dst_nb_samples = av_rescale_rnd(swr_get_delay(m_context.get(), m_input_sample_rate) +
		m_src_nb_samples, m_output_sample_rate, m_input_sample_rate, AV_ROUND_UP);

	if (dst_nb_samples > max_dst_nb_samples) {
		av_freep(&dst_data[0]);
		auto ret = av_samples_alloc(dst_data, &dst_linesize, m_output_channels,
			dst_nb_samples, m_output_format, 1);
		if (ret < 0)
			return -1;
		max_dst_nb_samples = dst_nb_samples;
	}
	

	int32_t real_output_samples = swr_convert(m_context.get(), dst_data, dst_nb_samples,
		(const uint8_t**)src_data, m_src_nb_samples);

	*output_samples = real_output_samples;

	if (real_output_samples < 0) {
		return real_output_samples;
	}

	auto dst_bufsize = av_samples_get_buffer_size(&dst_linesize, m_output_channels, real_output_samples,
		m_output_format, 1);

	if (dst_bufsize <0)
	{
		return -1;
	}
	// 确保 output_data 指向的内存已经被分配
	if (output_buffer != nullptr) {

		if (m_output_format < AV_SAMPLE_FMT_U8P)
		{
			for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
				if (output_buffer[i] == nullptr)
				{
					output_buffer[i] = new uint8_t[dst_bufsize];
				}
			}

			if (dst_data[0] != nullptr)
			{
				memcpy(output_buffer[0], dst_data[0], dst_bufsize);
#if DUMP_AUDIO
				fwrite((char*)output_buffer[0], 1, dst_bufsize, decode_file);
#endif
			}
		}
		else
		{

			for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
				if (output_buffer[i] == nullptr)
				{
					output_buffer[i] = new uint8_t[dst_bufsize / m_output_channels];
				}
			}
			for (int i = 0; i < m_output_channels; ++i) {
				if (output_buffer[i] == nullptr)
				{
					output_buffer[i] = new uint8_t[dst_bufsize / m_output_channels];
				}
				if (dst_data[i] != nullptr)
				{
					memcpy(output_buffer[i], dst_data[i], dst_bufsize / m_output_channels);
#if DUMP_AUDIO
					fwrite((char*)dst_data[i], 1, dst_bufsize / m_output_channels, decode_file);
#endif
				}
			}
		}

	//	output_buffer = new uint8_t * [m_output_channels];
		//init_converted_samples(&output_buffer, m_output_channels,m_output_format, m_src_nb_samples);
	}

	if (bAddfifo)
	{
		if (dst_bufsize > 0)
		{
			 AddToFifo((void**)dst_data, real_output_samples);
		}

	}

	return dst_bufsize;

}

int AudioResampler::ResampleAudio(uint8_t** iBuffer, int iLength)
{
	int ret = -1;
	if (m_convert)
	{
		// 填充输入帧的数据
		uint8_t** output_data = nullptr;
	//	init_converted_samples(&output_data, m_output_channels, m_output_format, m_src_nb_samples);
		int32_t output_samples = 0;


		auto output_size = Resample(output_data, &output_samples, (const uint8_t**)iBuffer, iLength, true);

		if (output_size > 0)
		{
			ret = 0;
		}
		// 释放 output_data 数组
		if (output_data)
		{
			for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i)
			{
				if (output_data[i]!=nullptr)
				{
					delete[] output_data[i];
					output_data[i] = nullptr;
				}
			
			}
			delete[] output_data;
			output_data = nullptr;  // 将指针设置为 nullptr，以避免悬挂指针
		}
	}
	else
	{
		int input_nb_samples = iLength / m_input_channels / av_get_bytes_per_sample(m_input_format);
		//uint8_t* data[2];
		//data[0] = iBuffer;
		//data[1] = iBuffer + input_nb_samples * av_get_bytes_per_sample(m_input_format);
		ret = AddToFifo((void**)iBuffer, input_nb_samples);
	}
	return ret;

}
int AudioResampler::FillingPCM(uint8_t* iBuffer[AV_NUM_DATA_POINTERS], int iLength)
{
	return ResampleAudio((uint8_t**)iBuffer, iLength);
}
int AudioResampler::GetOneFrame(uint8_t* oBuffer[AV_NUM_DATA_POINTERS], int* oLength)
{
	AVAudioFifo* fifo = m_pcm_fifo;
	int frame_size = av_audio_fifo_size(fifo);
	//std::cout << "frame_size before" << frame_size << std::endl;
	if (frame_size < m_out_frame_size)
		return -1;
	int out_size = av_samples_get_buffer_size(nullptr,m_output_channels , m_out_frame_size,m_output_format , 0);
	/* Read as many samples from the FIFO buffer as required to fill the frame.
	 * The samples are stored in the frame temporarily. */
	//uint8_t* data[AV_NUM_DATA_POINTERS] = { 0 };
	//data[0] = oBuffer;
	//data[1] = oBuffer + out_size / m_output_channels;
	if (out_size<0)
	{
		return -1;
	}

	for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
		if (oBuffer[i] == nullptr)
		{
			oBuffer[i] = new uint8_t[out_size];
		}

	}


	int ret = av_audio_fifo_read(fifo, (void**)oBuffer, m_out_frame_size);
	if (ret < m_out_frame_size)
	{
		for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
			if (oBuffer[i] != nullptr)
			{
				delete[]oBuffer[i];
				oBuffer[i] = nullptr;			
			}
		}
		return -1;
	}
	*oLength = out_size;
	frame_size = av_audio_fifo_size(fifo);
	//std::cout << "frame_size" << frame_size << std::endl;
	return 0;
}

int AudioResampler::AddToFifo(void** data, int size)
{
	if (size <= 0)
		return -1;
	
	int future_size = av_audio_fifo_size(m_pcm_fifo) + size;
	if (av_audio_fifo_space(m_pcm_fifo) < size)
	{
		if ((av_audio_fifo_space(m_pcm_fifo) + future_size) < m_current_max_fifo_size)//扩容
		{
			if (av_audio_fifo_realloc(m_pcm_fifo, m_current_max_fifo_size) < 0) {
				SPDLOG_LOGGER_WARN(spdlogptr,"Could not reallocate FIFO");
				return -1;
			}
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlogptr,"Out Max Fifo value");		
			return -1;
		}
	}
	/* Store the new samples in the FIFO buffer. */
	if (av_audio_fifo_write(m_pcm_fifo, data, size) < size) {
		SPDLOG_LOGGER_ERROR(spdlogptr,"Could not write data to FIFO");
		return -1;
	}
	return 0;
}

bool AudioResampler::CreatePcmFifo()
{
	int frame = 1024 * 20;
	m_pcm_fifo = av_audio_fifo_alloc(m_output_format, m_output_channels, frame);
	if (m_pcm_fifo == nullptr)
	{
		return false;
	}
	m_current_max_fifo_size = frame * 10;
	return true;
}
int AudioResampler::ResampleAndFifo(uint8_t* iBuffer, int iLength)
{
	//int input_nb_samples = iLength / m_input_channels / av_get_bytes_per_sample(m_input_format);
	//int out_count = av_rescale_rnd(swr_get_delay(m_audio_convert_ctx, m_input_sample) +
	//	input_nb_samples, m_out_sample, m_input_sample, AV_ROUND_UP);
	//int ret = 0;
	//if (m_one_frame_bffer == nullptr)
	//{
	//	uint8_t** buffer = static_cast<uint8_t**>(calloc(m_output_channels, sizeof(uint8_t**)));
	//	if (buffer == nullptr)
	//	{
	//		//	LOG(ERROR) << "m_one_frame_bffer error";
	//		return -1;
	//	}
	//	ret = av_samples_alloc(buffer, nullptr, m_output_channels, out_count, m_output_format, 0);
	//	if (ret < 0)
	//	{
	//		//LOG(ERROR) << "m_one_frame_bffer error";
	//		return -1;
	//	}
	//	m_one_frame_bffer = buffer;
	//}
	//ret = swr_convert(m_audio_convert_ctx, m_one_frame_bffer, out_count, (const uint8_t**)&iBuffer, input_nb_samples);
	//if (ret < 0) {
	//	//LOG(WARNING) << "DoResample swr_convert error";
	//	return -1;
	//}
	//ret = AddToFifo((void**)m_one_frame_bffer, ret);

	return 0;
}
//int AudioResampler::InitFromS16ToFLTP(int in_channels, int in_sample_rate, int out_channels, int out_sample_rate)
//{
//	in_channels_ = in_channels;
//	in_sample_rate_ = in_sample_rate;
//	out_channels_ = out_channels;
//	out_sample_rate_ = out_sample_rate;
//#ifdef FFMPEG6
//	AVChannelLayout in_ch_layout = {}, out_ch_layout = {};
//	out_ch_layout.nb_channels = out_channels_;
//	in_ch_layout.nb_channels = in_channels_;
//	av_channel_layout_default(&out_ch_layout, out_channels_);
//	av_channel_layout_default(&in_ch_layout, in_channels_);
//	auto res = swr_alloc_set_opts2(&ctx_,
//		&out_ch_layout,
//		AV_SAMPLE_FMT_FLTP,
//		out_sample_rate_,
//		&in_ch_layout,
//		AV_SAMPLE_FMT_S16,
//		in_sample_rate_,
//		0,
//		NULL);
//	if (!ctx_) {
//		printf("swr_alloc_set_opts failed\n");
//		return -1;
//	}
//	int ret = swr_init(ctx_);
//	if (ret < 0) {
//		char errbuf[1024] = { 0 };
//		av_strerror(ret, errbuf, sizeof(errbuf) - 1);
//		printf("swr_init  failed:%s\n", errbuf);
//		return -1;
//	}
//	return 0;
//
//
//#else
//
//	ctx_ = swr_alloc_set_opts(ctx_,
//		av_get_default_channel_layout(out_channels_),
//		AV_SAMPLE_FMT_FLTP,
//		out_sample_rate_,
//		av_get_default_channel_layout(in_channels_),
//		AV_SAMPLE_FMT_S16,
//		in_sample_rate_,
//		0,
//		NULL);
//	if (!ctx_) {
//		printf("swr_alloc_set_opts failed\n");
//		return -1;
//	}
//	int ret = swr_init(ctx_);
//	if (ret < 0) {
//		char errbuf[1024] = { 0 };
//		av_strerror(ret, errbuf, sizeof(errbuf) - 1);
//		printf("swr_init  failed:%s\n", errbuf);
//		return -1;
//	}
//	return 0;
//#endif // FFMPEG6
//
//
//}
//
//int AudioResampler::ResampleFromS16ToFLTP(uint8_t* in_data, AVFrame* out_frame)
//{
//	const uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
//	indata[0] = in_data;
//	int samples = swr_convert(ctx_, out_frame->data, out_frame->nb_samples,
//		indata, out_frame->nb_samples);
//	if (samples <= 0) {
//		return -1;
//	}
//	return samples;
//}
//
//AVFrame* AllocFltpPcmFrame(int channels, int nb_samples)
//{
//#ifdef FFMPEG6
//	AVFrame* pcm = NULL;
//	pcm = av_frame_alloc();
//	pcm->format = AV_SAMPLE_FMT_FLTP;
//	pcm->ch_layout.nb_channels = channels;
//	av_channel_layout_default(&(pcm->ch_layout), channels);
//	pcm->nb_samples = nb_samples;
//
//	int ret = av_frame_get_buffer(pcm, 0);
//	if (ret != 0) {
//		char errbuf[1024] = { 0 };
//		av_strerror(ret, errbuf, sizeof(errbuf) - 1);
//		printf("av_frame_get_buffer failed:%s\n", errbuf);
//		av_frame_free(&pcm);
//		return NULL;
//	}
//	return pcm;
//
//#else
//	AVFrame* pcm = NULL;
//	pcm = av_frame_alloc();
//	pcm->format = AV_SAMPLE_FMT_FLTP;
//	pcm->channels = channels;
//	pcm->channel_layout = av_get_default_channel_layout(channels);
//	pcm->nb_samples = nb_samples;
//
//	int ret = av_frame_get_buffer(pcm, 0);
//	if (ret != 0) {
//		char errbuf[1024] = { 0 };
//		av_strerror(ret, errbuf, sizeof(errbuf) - 1);
//		printf("av_frame_get_buffer failed:%s\n", errbuf);
//		av_frame_free(&pcm);
//		return NULL;
//	}
//	return pcm;
//#endif // FFMPEG6
//
//
//}
//
//void FreePcmFrame(AVFrame* frame)
//{
//	if (frame)
//		av_frame_free(&frame);
//}


}




//
//AudioResampler::AudioResampler(
//	AVSampleFormat input_format, int32_t input_sample_rate, int32_t input_channel_num,
//	AVSampleFormat output_format, int32_t output_sample_rate, int32_t output_channel_num
//)
//	: m_context(swr_alloc()),
//	m_input_format(input_format),
//	m_input_sample_rate(input_sample_rate),
//	m_input_channel_num(input_channel_num),
//	m_output_format(output_format),
//	m_output_sample_rate(output_sample_rate),
//	m_output_channel_num(output_channel_num)
//{
//
//
//
//	SwrContext* contextPtr = m_context.get();
//
//	/* set options */
//	//AVChannelLayout src_ch_layout = (m_input_channel_num == 1) ? AV_CHANNEL_LAYOUT_MONO : AV_CHANNEL_LAYOUT_STEREO;
//	//AVChannelLayout src_ch_layout = AV_CHANNEL_LAYOUT_STEREO, dst_ch_layout = AV_CHANNEL_LAYOUT_SURROUND;
//
//	//AVChannelLayout src_ch_layout , dst_ch_layout ;
//	//src_ch_layout.nb_channels = m_input_channel_num;
//	//src_ch_layout.u.mask = (m_input_channel_num == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
//	//src_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
//	//dst_ch_layout.nb_channels = output_channel_num;
//	//dst_ch_layout.u.mask = (m_input_channel_num == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
//	//dst_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
//
//	//av_opt_set_chlayout(contextPtr, "in_chlayout", &src_ch_layout, 0);
//	//av_opt_set_int(contextPtr, "in_sample_rate", m_input_sample_rate, 0);
//	//av_opt_set_sample_fmt(contextPtr, "in_sample_fmt", input_format, 0);
//
//	//av_opt_set_chlayout(contextPtr, "out_chlayout", &dst_ch_layout, 0);
//	//av_opt_set_int(contextPtr, "out_sample_rate", output_sample_rate, 0);
//	//av_opt_set_sample_fmt(contextPtr, "out_sample_fmt", output_format, 0);
//
// // 初始化 SwrContext
//	AVChannelLayout out_ch_layout;
//	out_ch_layout.nb_channels = output_channel_num;
//	out_ch_layout.u.mask = (output_channel_num == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
//	out_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
//
//	AVChannelLayout inChannelLayout;
//	inChannelLayout.nb_channels = input_channel_num;
//	inChannelLayout.u.mask = (m_input_channel_num == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
//	inChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
//
//
//	int32_t error = swr_alloc_set_opts2(&contextPtr,
//		&out_ch_layout,
//		output_format,
//		output_sample_rate,
//		&inChannelLayout,
//		input_format,
//		input_sample_rate,
//		0,
//		0);
//	if (error < 0) {
//		std::cerr << "Failed to allocate SwrContext" << std::endl;
//		// 处理错误，可以抛出异常或采取其他措施
//	}
//
//	error = swr_init(m_context.get());
//	if (error < 0) {
//		std::cerr << "Failed to initialize SwrContext" << std::endl;
//		// 处理错误，可以抛出异常或采取其他措施
//	}
//	auto ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, output_channel_num,
//		output_sample_rate, output_format, 1);
//	if (ret < 0) {
//		fprintf(stderr, "Could not allocate source samples\n");
//		if (dst_data)
//			av_freep(&dst_data[0]);
//		av_freep(&dst_data);
//	}
//
//
//}
//
//int32_t AudioResampler::Resample(const uint8_t** src_data, int in_count, uint8_t** output_buffer) {
//
//	int needed_buf_size = av_samples_get_buffer_size(nullptr,
//		m_output_channel_num,
//		m_output_sample_rate,
//		m_output_format, ALIGNMENT);
//
//	if (*output_buffer == nullptr)
//	{
//		*output_buffer = new uint8_t[dst_linesize];
//	}
//
//	int32_t real_output_samples = swr_convert(
//		m_context.get(), dst_data, dst_linesize,
//		(const uint8_t**)src_data, src_nb_samples
//	);
//
//	if (real_output_samples < 0) {
//		return 0;
//	}
//
//	auto real_output_size = av_samples_get_buffer_size(nullptr, m_output_channel_num, real_output_samples,
//		m_output_format, 1);
//	memcpy(*output_buffer, dst_data[0], real_output_size);
//	return real_output_size;
//}
