#pragma once
/**
 *　Windows屏幕录像模块
 */
#include <mutex>
#include <memory>
#include <atomic>

#include <api/scoped_refptr.h>
#include "rtc_base/thread.h"
#include "pc/local_audio_source.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "rtc_base/ref_counted_object.h"
#include "../capture/AudioCapture.h"

 /*

	 rtc::scoped_refptr<AudioTrackSourceInput> audioSource;
	 m_audioDeviceModule->Terminate();
	 audioSource = AudioTrackSourceInput::Create(audiourl, opts);
	 return audioSource;


 */
class AudioTrackSourceInput : public webrtc::Notifier<webrtc::AudioSourceInterface>
{
public:
	static rtc::scoped_refptr<AudioTrackSourceInput> Create(const std::string& uri, const std::map<std::string, std::string>& opts) {
		rtc::scoped_refptr<AudioTrackSourceInput> source(new rtc::RefCountedObject<AudioTrackSourceInput>(uri, opts, true));
		return source;
	}

	SourceState state() const override { return kLive; }


	bool remote() const override { return true; }

	virtual void AddSink(webrtc::AudioTrackSinkInterface* sink) override;

	virtual void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override;

	//发送音频裸流
	int32_t SendRecordedBuffer(int8_t* audio_data,
		uint32_t data_len,
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels
		, int64_t sourcets);

protected:
	AudioTrackSourceInput(const std::string& uri, const std::map<std::string, std::string>& opts, bool wait);


	virtual ~AudioTrackSourceInput();

private:
	AudioCapture* m_pAudioCapture = nullptr;
	rtc::scoped_refptr<webrtc::AudioDecoderFactory> m_factory;

	std::vector<uint16_t> m_vecbuffer;

	std::list<webrtc::AudioTrackSinkInterface*> m_sinks;
	std::mutex m_sink_lock;
	bool m_wait;
	int64_t m_previmagets;
	int64_t m_prevts;

	std::string m_audiourl;
};
