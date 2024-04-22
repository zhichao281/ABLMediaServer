#include "AudioDecoder.h"
#include "spdloghead.h"

// 将FFmpeg的错误码转换为字符串
std::string ffmpegErrorToString(int errorCode) {
	char errorString[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_make_error_string(errorString, AV_ERROR_MAX_STRING_SIZE, errorCode);
	return std::string(errorString);
}
AudioDecoder::AudioDecoder(std::string par, std::function <void(uint8_t** data, int size)> result_callback)
	:m_success(false),
	m_callback(result_callback),
	m_codec_context(nullptr)
{
	SPDLOG_LOGGER_INFO(spdlogptr,"AudioDecode={}", par);
	//NSJsonObject json_par = NSJson::ParseStr(par);
	//m_channels = json_par.OptGetInt("channels", 2);
	//m_sample_rate = json_par.OptGetInt("sample_rate", 48000);
	//m_channel_layout = json_par.OptGetInt("channel_layout", av_get_default_channel_layout(2));
	//m_codec_id = static_cast<AVCodecID>(json_par.OptGetInt("id", AV_CODEC_ID_AAC));
	//m_enc_name = json_par.OptGetString("enc_name", "");
	//m_bit_rate = json_par.OptGetInt("bit_rate", 128000);
	//m_format = json_par.OptGetInt("format", -1);
}
AudioDecoder::AudioDecoder(const std::map<std::string, std::string>& opts)
	: m_success(false), m_codec_context(nullptr) {

	// 从选项中获取音频流信息
	auto streamInfo = opts;
	m_format = atoi(streamInfo["format"].c_str());
	m_sample_rate = atoi(streamInfo["sample_rate"].c_str());
	m_nb_channels = atoi(streamInfo["nb_channels"].c_str());
	m_codec_id = (AVCodecID)atoi(streamInfo["codec_id"].c_str());
	m_bit_rate = atoi(streamInfo["bit_rate"].c_str());
	auto strName = streamInfo["audiocodecname"];


	// 输出音频流信息
	SPDLOG_LOGGER_INFO(spdlogptr, "m_format={}, m_sample_rate={}, m_nb_channels={}, m_codec_id={}, m_bit_rate={}",
		m_format, m_sample_rate, m_nb_channels, (int)m_codec_id, m_bit_rate);

	const AVCodec* input_codec;
	if (m_codec_id == 0)
	{
		// 查找音频解码器
		 input_codec = avcodec_find_decoder_by_name(strName.c_str());
		if (!input_codec) {
			SPDLOG_LOGGER_ERROR(spdlogptr, "avcodec_find_decoder Error for id:{}", (int)m_codec_id);
			return;
		}
	}
	else
	{
		// 查找音频解码器
		 input_codec = avcodec_find_decoder((AVCodecID)m_codec_id);
		if (!input_codec) {
			SPDLOG_LOGGER_ERROR(spdlogptr, "avcodec_find_decoder Error for id:{}", (int)m_codec_id);
			return;
		}
	}


	// 分配解码器上下文
	m_codec_context = avcodec_alloc_context3(input_codec);
	if (!m_codec_context) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "avcodec_alloc_context3 Error for id: {}", (int)m_codec_id);
		return;
	}
	// 设置解码器参数
	//m_codec_context->bit_rate = m_bit_rate;
	m_codec_context->sample_rate = m_sample_rate;
	av_channel_layout_default(&(m_codec_context->ch_layout), m_nb_channels);
	m_codec_context->sample_fmt = static_cast<AVSampleFormat>(m_format);

	// 打开解码器
	int ret = avcodec_open2(m_codec_context, input_codec, nullptr);
	if (ret < 0) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "avcodec_open2 Error for id: {} error: {}", (int)m_codec_id, ffmpegErrorToString(ret));
		avcodec_free_context(&m_codec_context);
		m_codec_context = nullptr;
		return;
	}

	m_success = true;
}


AudioDecoder::~AudioDecoder()
{
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioDecode  start={} ", static_cast<void*>(this));
	m_Framecallback = nullptr;
	m_callback = nullptr;
	m_success = false;
	if (m_codec_context)
	{
		avcodec_free_context(&m_codec_context);
		m_codec_context = nullptr;
	}
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioDecode end ={} ", static_cast<void*>(this));
}


bool AudioDecoder::DecodeData(uint8_t* inData, int inSize, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)
{
	if (!m_success || !inData || inSize <= 0 ) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: Invalid input parameters");
		return false;
	}
	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	if (!packet || !frame) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: Failed to allocate packet or frame");
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}

	// 设置AVPacket的数据和大小
	packet->data = inData;
	packet->size = inSize;

	// 将数据包发送到解码器
	int ret = avcodec_send_packet(m_codec_context, packet);
	if (ret < 0)
	{	
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: avcodec_send_packet error: {}", ffmpegErrorToString(ret));
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}


	// 从解码器接收解码后的帧
	ret = avcodec_receive_frame(m_codec_context, frame);

	if (ret < 0) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: avcodec_receive_frame error: {}", ffmpegErrorToString(ret));
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}

	if (ret == 0)
	{
		for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) 
		{
			if (outData[i]==nullptr)
			{
				outData[i] = new uint8_t[frame->linesize[0]];
			}
			if (frame->data[i] != NULL) 
			{	// 复制音频帧的数据到 outData 数组中
				memcpy(outData[i], frame->data[i], frame->linesize[0]);
			}
		}
		if (m_callback)
		{
			m_callback(frame->data, frame->nb_samples);
		}
		(*outSize) = frame->nb_samples;
	}
	av_packet_free(&packet);
	av_frame_free(&frame);
	return true ;
}
bool AudioDecoder::DecodeData(AVPacket* packet, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)
{
	return DecodeData(packet->data, packet->size,outData,outSize);

}
bool AudioDecoder::DecodeData(uint8_t* inData, int inSize)
{
	int outSize = 0;
	uint8_t* outData[AV_NUM_DATA_POINTERS] = { 0 };
	auto res= DecodeData(inData, inSize, outData, &outSize);
	for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
		if (outData[i] != nullptr) {
			delete[] outData[i];
			outData[i] = nullptr;
		}
	}
	return res;
}
bool AudioDecoder::PushAudioPacket(AVPacket* packet, AVFrame* frame)
{
	if (!m_success) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: Invalid input parameters");
		return false;
	}

	if (!packet || !frame) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: Failed to allocate packet or frame");
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}

	// 将数据包发送到解码器
	int ret = avcodec_send_packet(m_codec_context, packet);
	if (ret < 0)
	{
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: avcodec_send_packet error: {}", ffmpegErrorToString(ret));
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}


	// 从解码器接收解码后的帧
	ret = avcodec_receive_frame(m_codec_context, frame);
	if (ret < 0) {
		SPDLOG_LOGGER_ERROR(spdlogptr, "DecodeAudio: avcodec_receive_frame error: {}", ffmpegErrorToString(ret));
		av_packet_free(&packet);
		av_frame_free(&frame);
		return false;
	}

	return true;

}

bool AudioDecoder::PushAudioPacket(uint8_t* aBytes, int aSize, int64_t ts)
{
	return true;
}

void AudioDecoder::RegisterDecodeCallback(std::function<void(AVFrame* data)> result_callback)
{
	m_Framecallback = result_callback;
}
AVFrame* AudioDecoder::frame_alloc()
{
	return av_frame_alloc();
}
void AudioDecoder::RegisterDecodeCallback(std::function <void(uint8_t** data, int size)> result_callback)
{
	m_callback = result_callback;
};

