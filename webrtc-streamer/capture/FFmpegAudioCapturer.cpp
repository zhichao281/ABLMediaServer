#include "FFmpegAudioCapturer.h"

#include "spdloghead.h"
#ifndef WIN32
#include <malloc.h>
#endif
FFmpegAudioSource::FFmpegAudioSource(const std::string& uri, const std::map<std::string, std::string>& opts)
	:m_audioourl(uri)
{
	m_bStop.store(true);
	stopThread.store(false);
}

FFmpegAudioSource::~FFmpegAudioSource()
{
	SPDLOG_LOGGER_ERROR(spdlogptr, "stop start ");

#ifndef WIN32
	// 释放一些内存回给系统
	malloc_trim(0);
#endif

	SPDLOG_LOGGER_ERROR(spdlogptr, "stop end ");


}

int FFmpegAudioSource::Init(const char* audiocodecname, int nSampleRate, int nChannel, int nBitsPerSample)
{

	return 0;
}

int FFmpegAudioSource::Init(const std::map<std::string, std::string>& opts)
{
	SPDLOG_LOGGER_INFO(spdlogptr, "OnSourceConnected ");
	auto streamInfo = opts;
	auto  fmat = atoi(streamInfo["format"].c_str());
	auto  sample_rate = atoi(streamInfo["sample_rate"].c_str());
	auto  nb_channels = atoi(streamInfo["nb_channels"].c_str());
	auto  audiocodecname = streamInfo["audiocodecname"].c_str();
	auto  bit_rate = atoi(streamInfo["bit_rate"].c_str());

	auto  frame_size = atoi(streamInfo["frame_size"].c_str());
	return true;
}

int FFmpegAudioSource::Start()
{
	SPDLOG_LOGGER_INFO(spdlogptr, "Start ");
	if (m_bStop.load() == true)
	{	
		m_bStop.store(false);
		return true;
	}
	else
	{
		return false;
	}


}

void FFmpegAudioSource::Stop()
{
}

void FFmpegAudioSource::Destroy()
{

}

void FFmpegAudioSource::RegisterPcmCallback(PcmCallBack pcmCallback)
{
	m_pcmCallback = pcmCallback;
}

void FFmpegAudioSource::RegisterAacCallback(AacCallBack aacCallBack)
{
	
}

bool FFmpegAudioSource::onData(const char* id, unsigned char* buffer, ssize_t size, int64_t sourcets)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_pcmCallback)
	{
		m_pcmCallback(buffer, size, m_sample_rate, m_nb_channels, 0);
	}

	return true;
}
