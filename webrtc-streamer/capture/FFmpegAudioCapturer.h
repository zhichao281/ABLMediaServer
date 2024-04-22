/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FFmpegAudioSource.h
**
** -------------------------------------------------------------------------*/
// 是否支持ffmpeg功能

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <cctype>
#include <stdio.h>
#include <string.h>
#include <condition_variable>
#include "pc/local_audio_source.h"

#include "AudioCapture.h"

typedef unsigned char uchar;
typedef unsigned int uint;


class FFmpegAudioSource : public AudioCapture
{
public:
	FFmpegAudioSource(const std::string& uri, const std::map<std::string, std::string>& opts);
	~FFmpegAudioSource();
	virtual int Init(const char* audiocodecname, int nSampleRate, int nChannel, int nBitsPerSample) override;

	virtual int Init(const std::map<std::string, std::string>& opts) override;

	virtual int Start() override;

	virtual void Stop() override;

	virtual void Destroy() override;

	virtual void RegisterPcmCallback(PcmCallBack pcmCallback);

	virtual void RegisterAacCallback(AacCallBack aacCallBack);

	virtual bool onData(const char* id, unsigned char* buffer, ssize_t size, int64_t sourcets);
	virtual void HangUp() {};

	virtual void WakeUp() {};


private:

	std::list<PcmCallBack> m_PcmCallbackList;
	std::atomic<bool>			m_bStop;
	std::mutex					m_mutex;                //互斥锁	
	AacCallBack				m_AACCallback;
	PcmCallBack m_pcmCallback;
	std::string				    m_audioourl;
	std::map<std::string, std::string> m_codec;

	std::condition_variable audioDataCV;

	std::atomic<bool> stopThread;

	int m_nb_channels=2;

	int m_sample_rate=48000;
};
