

#include "AudioCapture.h"
#include <thread>

#include "FFmpegAudioCapturer.h"
AudioCapture* AudioCapture::CreateAudioCapture(std::string audiourl, const std::map<std::string, std::string> opts)
{
	return new FFmpegAudioSource(audiourl, opts);
}




// 添加输入流
void AudioCaptureManager::AddInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_inputMap.count(videoUrl) == 0)
	{
		AudioCapture* input = AudioCapture::CreateAudioCapture(videoUrl);
		if (input)
		{
			m_inputMap[videoUrl] = input;
		}
	}

}

// 移除输入流
void AudioCaptureManager::RemoveInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string path = videoUrl;
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		m_inputMap.erase(it);
	}
}

// 获取输入流对象
AudioCapture* AudioCaptureManager::GetInput(const std::string& videoUrl)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string  path = videoUrl;
	auto it = m_inputMap.find(path);
	if (it != m_inputMap.end())
	{
		return it->second;
	}
	else
	{
		AudioCapture* input = AudioCapture::CreateAudioCapture(path);
		if (input)
		{
			m_inputMap[path] = input;
		}
		return input;
	}
}


std::string AudioCaptureManager::getStream(const std::string& videoUrl)
{


	std::string path = "";
	if (isURLWithProtocol(videoUrl)) {
		//std::cout << "Input string is a URL with protocol." << std::endl;
		path = getPortionAfterPort(videoUrl);
		//	std::cout << "Extracted path: " << path << std::endl;
	}
	else {
		//std::cout << "Input string is a simple path." << std::endl;
		path = videoUrl;
	}
	return videoUrl;
}

AudioCaptureManager& AudioCaptureManager::getInstance()
{
	static AudioCaptureManager instance;
	return instance;
}
