#include "AudioTrackSourceInput.h"
#include "spdloghead.h"


void AudioTrackSourceInput::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioTrackSourceInput::AddSink ");
	std::lock_guard<std::mutex> lock(m_sink_lock);
	m_sinks.push_back(sink);
}

void AudioTrackSourceInput::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioTrackSourceInput::RemoveSink ");
	std::lock_guard<std::mutex> lock(m_sink_lock);
	m_sinks.remove(sink);
}

//发送音频裸流
int32_t AudioTrackSourceInput::SendRecordedBuffer(int8_t* audio_data,
	uint32_t data_len,
	int bits_per_sample,
	int sample_rate,
	size_t number_of_channels
	, int64_t sourcets) {


	int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
	if ((m_wait) && (m_prevts != 0))
	{
		int64_t periodSource = sourcets - m_previmagets;
		int64_t periodDecode = ts - m_prevts;


		int64_t delayms = periodSource - periodDecode;

		if ((delayms > 0) && (delayms < 1000))
		{
			//	 std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}

		//SPDLOG_LOGGER_ERROR(spdlogptr, "FFmpegAudioSource::onData interframe decode: {} ", periodDecode);
	}
	std::vector<int16_t> newdecoded(reinterpret_cast<int16_t*>(audio_data), reinterpret_cast<int16_t*>(audio_data + data_len));
	m_vecbuffer.insert(m_vecbuffer.end(), newdecoded.begin(), newdecoded.end());
	newdecoded.clear();
	/* int16_t* newdecoded = (int16_t*)audio_data;
	 if (data_len > 0)
	 {
		 for (int i = 0; i < data_len / 2; )
		 {
			 m_vecbuffer.push_back((int16_t)(newdecoded)[i]);
			 i++;
		 }
	 }*/
	 //delete audio_data;
	 //audio_data = nullptr;
	int segmentLength = sample_rate / 100;
	int size = segmentLength * number_of_channels;
	while (m_vecbuffer.size() > size)
	{
		//std::vector<int16_t> segmentData(m_vecbuffer.begin(), m_vecbuffer.begin() + size);
		std::lock_guard<std::mutex> lock(m_sink_lock);
		if (m_sinks.size() < 1)
		{
			return -1;
		}
		for (auto* sink : m_sinks)
		{
			// 在这里处理音频数据，例如将数据发送给网络
	   // audio_data 为 PCM 数据缓冲区地址
	   // bits_per_sample 为样本位数
	   // sample_rate 为采样率
	   // number_of_channels 为声道数
	   // number_of_frames 为每个声道的采样帧数		
		   // sink->OnData(segmentData.data(), bits_per_sample, sample_rate, number_of_channels, segmentLength);
			sink->OnData(&m_vecbuffer[0], 16, sample_rate, number_of_channels, segmentLength);
		}
		m_vecbuffer.erase(m_vecbuffer.begin(), m_vecbuffer.begin() + size);
	}


	m_previmagets = sourcets;
	m_prevts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
	return 0;

};


AudioTrackSourceInput::AudioTrackSourceInput(const std::string& uri, const std::map<std::string, std::string>& opts, bool wait)
	:m_wait(wait)
	, m_previmagets(0)
	, m_prevts(0)
	, m_audiourl(uri)
{

	m_pAudioCapture =AudioCaptureManager::getInstance().GetInput(uri); 
	if (m_pAudioCapture)
	{
		m_pAudioCapture->Init("",0, 0, 0);
		m_pAudioCapture->RegisterPcmCallback([=](uint8_t* pcm, int datalen, int nSampleRate, int nChannel, int64_t nTimeStamp)
			{
				//  std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
				//	  _worker_thread_ptr->Start();
				//	  _worker_thread_ptr->PostTask([&]()
					//	  {
				SendRecordedBuffer((int8_t*)pcm, (uint32_t)datalen, 16, nSampleRate, (size_t)nChannel, nTimeStamp);

				// }
			// );


			});
		m_pAudioCapture->Start();
	}


}

AudioTrackSourceInput::~AudioTrackSourceInput()
{
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioTrackSourceInput::stop start ");
	if (m_pAudioCapture)
	{
		AudioCaptureManager::getInstance().RemoveInput(m_audiourl);
		m_pAudioCapture->Stop();
		delete m_pAudioCapture;
		m_pAudioCapture = nullptr;
	}
	{
		std::lock_guard<std::mutex> lock(m_sink_lock);
		m_sinks.clear();
		m_vecbuffer.clear();
	}
	SPDLOG_LOGGER_INFO(spdlogptr, "AudioTrackSourceInput::stop end ");

}