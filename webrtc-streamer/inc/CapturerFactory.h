/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** CapturerFactory.h
**
** -------------------------------------------------------------------------*/

#pragma once

#include <regex>

#include "VcmCapturer.h"

#ifdef HAVE_V4L2
#include "V4l2Capturer.h"
#endif

#ifdef HAVE_LIVE555
#include "rtspvideocapturer.h"
#include "rtpvideocapturer.h"
#include "filevideocapturer.h"
#include "rtspaudiocapturer.h"
#include "fileaudiocapturer.h"
#endif

#ifdef USE_X11
#include "screencapturer.h"
#include "windowcapturer.h"
#endif

#ifdef HAVE_RTMP
#include "rtmpvideosource.h"
#endif

#include "spdloghead.h"
#include "pc/video_track_source.h"
#include "VideoTrackSourceInput.h"
#include "AudioTrackSourceInput.h"
#ifdef _WIN32
class GBK
{
public:
	GBK() {};
	~GBK() {};
	static std::string GbkToUtf8(const char* src_str)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
		wchar_t* wstr = new wchar_t[len + 1];
		memset(wstr, 0, len + 1);
		MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
		len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* str = new char[len + 1];
		memset(str, 0, len + 1);
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
		std::string strTemp = str;
		if (wstr) delete[] wstr;
		if (str) delete[] str;
		return strTemp;
	}
	static std::string Utf8ToGbk(const char* src_str)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
		wchar_t* wszGBK = new wchar_t[len + 1];
		memset(wszGBK, 0, len * 2 + 2);
		MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
		len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
		char* szGBK = new char[len + 1];
		memset(szGBK, 0, len + 1);
		WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
		std::string strTemp(szGBK);
		if (wszGBK) delete[] wszGBK;
		if (szGBK) delete[] szGBK;
		return strTemp;
	}
private:

};

#endif
template<class T>
class TrackSource : public webrtc::VideoTrackSource {
public:
	static rtc::scoped_refptr<TrackSource> Create(const std::string& videourl, const std::map<std::string, std::string>& opts, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory) {
		std::unique_ptr<T> capturer = absl::WrapUnique(T::Create(videourl, opts, videoDecoderFactory));
		if (!capturer) {
			return nullptr;
		}
		return rtc::make_ref_counted<TrackSource>(std::move(capturer));
	}

	virtual bool GetStats(Stats* stats) override {
		bool result = false;
		T* source = m_capturer.get();
		if (source) {
			stats->input_height = source->height();
			stats->input_width = source->width();
			result = true;
		}
		return result;
	}


protected:
	explicit TrackSource(std::unique_ptr<T> capturer)
		: webrtc::VideoTrackSource(/*remote=*/false), m_capturer(std::move(capturer)) {}

	SourceState state() const override {
		return kLive;
	}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
		return m_capturer.get();
	}
	std::unique_ptr<T> m_capturer;
};

class CapturerFactory {
public:

	static const std::list<std::string> GetVideoCaptureDeviceList(const std::regex& publishFilter, bool useNullCodec)
	{
		std::list<std::string> videoDeviceList;

		if (std::regex_match("videocap://", publishFilter) && !useNullCodec) {
			std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
			if (info)
			{
				int num_videoDevices = info->NumberOfDevices();
				SPDLOG_LOGGER_INFO(spdlogptr, "nb video devices:{}", num_videoDevices);
				for (int i = 0; i < num_videoDevices; ++i)
				{
					const uint32_t kSize = 256;
					char name[kSize] = { 0 };
					char id[kSize] = { 0 };
					if (info->GetDeviceName(i, name, kSize, id, kSize) != -1)
					{
						SPDLOG_LOGGER_INFO(spdlogptr, "video device name: {} ,id:{}", name, id);
						std::string devname;
						auto it = std::find(videoDeviceList.begin(), videoDeviceList.end(), name);
						if (it == videoDeviceList.end()) {
							devname = name;
						}
						else {
							devname = "videocap://";
							devname += std::to_string(i);
						}
						videoDeviceList.push_back(devname);
					}
				}
			}
		}
		if (std::regex_match("v4l2://", publishFilter) && useNullCodec) {
#ifdef HAVE_V4L2	
			DIR* dir = opendir("/dev");
			if (dir != nullptr) {
				struct dirent* entry = NULL;
				while ((entry = readdir(dir)) != NULL) {
					if (strncmp(entry->d_name, "video", 5) == 0) {
						std::string device("/dev/");
						device.append(entry->d_name);
						V4L2DeviceParameters param(device.c_str(), V4L2_PIX_FMT_H264, 0, 0, 0);
						V4l2Capture* capture = V4l2Capture::create(param);
						if (capture != NULL) {
							delete capture;
							std::string v4l2url("v4l2://");
							v4l2url.append(device);
							videoDeviceList.push_back(v4l2url);
						}
					}
				}
				closedir(dir);
			}
#endif		
		}

		return videoDeviceList;
	}

	static const std::map<int, std::string>  GetVideoCaptureDeviceMap()
	{

		std::map<int, std::string> videoMap;
#ifdef USE_X11
		std::unique_ptr<webrtc::DesktopCapturer> capturer = webrtc::DesktopCapturer::CreateWindowCapturer(webrtc::DesktopCaptureOptions::CreateDefault());
		if (capturer) {
			webrtc::DesktopCapturer::SourceList sourceList;
			if (capturer->GetSourceList(&sourceList)) {
				for (auto source : sourceList)
				{
					std::ostringstream os;
					os << "window://" << source.title;
					videoMap[source.id] = source.title;
				}
			}
		}
#endif		
		return videoMap;
	}


	static const std::list<std::string> GetVideoSourceList(const std::regex& publishFilter, bool useNullCodec) {

		std::list<std::string> videoList;

#ifdef USE_X11
		if (std::regex_match("window://", publishFilter) && !useNullCodec) {
			std::unique_ptr<webrtc::DesktopCapturer> capturer = webrtc::DesktopCapturer::CreateWindowCapturer(webrtc::DesktopCaptureOptions::CreateDefault());
			if (capturer) {
				webrtc::DesktopCapturer::SourceList sourceList;
				if (capturer->GetSourceList(&sourceList)) {
					for (auto source : sourceList) {
						std::ostringstream os;
						os << "window://" << source.title;
						videoList.push_back(os.str());
					}
				}
			}
		}
		if (std::regex_match("screen://", publishFilter) && !useNullCodec) {
			std::unique_ptr<webrtc::DesktopCapturer> capturer = webrtc::DesktopCapturer::CreateScreenCapturer(webrtc::DesktopCaptureOptions::CreateDefault());
			if (capturer) {
				webrtc::DesktopCapturer::SourceList sourceList;
				if (capturer->GetSourceList(&sourceList)) {
					for (auto source : sourceList) {
						std::ostringstream os;
						os << "screen://" << source.id;
						videoList.push_back(os.str());
					}
				}
			}
		}
#endif		
		return videoList;
	}

	static rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> CreateVideoSource(const std::string& videourl, const std::map<std::string, std::string>& opts, const std::regex& publishFilter, rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory, std::unique_ptr<webrtc::VideoDecoderFactory>& videoDecoderFactory) {
		rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource;
		videoSource = VideoTrackSourceInput::Create(videourl, opts);
		return videoSource;
		if (((videourl.find("rtsp://") == 0) || (videourl.find("rtsps://") == 0)) && (std::regex_match("rtsp://", publishFilter))) {
#ifdef HAVE_LIVE555
			videoSource = TrackSource<RTSPVideoCapturer>::Create(videourl, opts, videoDecoderFactory);

#endif	
			videoSource = VideoTrackSourceInput::Create(videourl, opts);
		}
		else if ((videourl.find("file://") == 0) && (std::regex_match("file://", publishFilter)))
		{
#ifdef HAVE_LIVE555
			videoSource = TrackSource<FileVideoCapturer>::Create(videourl, opts, videoDecoderFactory);
			//	videoSource = TrackSource<FFmpegVideoSource>::Create(videourl, opts, videoDecoderFactory);
#endif
			videoSource = VideoTrackSourceInput::Create(videourl, opts);
		}
		else if ((videourl.find("rtp://") == 0) && (std::regex_match("rtp://", publishFilter)))
		{
#ifdef HAVE_LIVE555
			videoSource = TrackSource<RTPVideoCapturer>::Create(SDPClient::getSdpFromRtpUrl(videourl), opts, videoDecoderFactory);
#endif
		}
		else if ((videourl.find("screen://") == 0) && (std::regex_match("screen://", publishFilter)))
		{
#ifdef USE_X11
			videoSource = TrackSource<ScreenCapturer>::Create(videourl, opts, videoDecoderFactory);
#endif	
		}
		else if ((videourl.find("window://") == 0) && (std::regex_match("window://", publishFilter)))
		{
#ifdef USE_X11
			videoSource = TrackSource<WindowCapturer>::Create(videourl, opts, videoDecoderFactory);
#endif 
		}
		else if ((videourl.find("rtmp://") == 0) && (std::regex_match("rtmp://", publishFilter))) {
#ifdef HAVE_RTMP
			videoSource = TrackSource<RtmpVideoSource>::Create(videourl, opts, videoDecoderFactory);
#endif 
			videoSource = VideoTrackSourceInput::Create(videourl, opts);
		}
		else if ((videourl.find("v4l2://") == 0) && (std::regex_match("v4l2://", publishFilter))) {
#ifdef HAVE_V4L2			
			videoSource = TrackSource<V4l2Capturer>::Create(videourl, opts, videoDecoderFactory);
#endif			
		}
		else if (std::regex_match("videocap://", publishFilter)) {
			videoSource = TrackSource<VcmCapturer>::Create(videourl, opts, videoDecoderFactory);


		}
		return videoSource;
	}

	static const std::list<std::string> GetAudioCaptureDeviceList(const std::regex& publishFilter, rtc::scoped_refptr<webrtc::AudioDeviceModule>   audioDeviceModule) {
		std::list<std::string> audioList;
		if (std::regex_match("audiocap://", publishFilter))
		{
			int16_t num_audioDevices = audioDeviceModule->RecordingDevices();
			SPDLOG_LOGGER_INFO(spdlogptr, "nb audio devices:{}", num_audioDevices);

			for (int i = 0; i < num_audioDevices; ++i)
			{
				char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
				char id[webrtc::kAdmMaxGuidSize] = { 0 };
				if (audioDeviceModule->RecordingDeviceName(i, name, id) != -1)
				{
					SPDLOG_LOGGER_INFO(spdlogptr, "audio device name:{}   id:{} ", name, id);
					std::string devname;
					auto it = std::find(audioList.begin(), audioList.end(), name);
					if (it == audioList.end()) {
						devname = name;
					}
					else {
						devname = "audiocap://";
						devname += std::to_string(i);
					}
					audioList.push_back(devname);
				}
			}
		}
		return audioList;
	}

	static const std::list<std::string> GetAudioPlayoutDeviceList(const std::regex& publishFilter, rtc::scoped_refptr<webrtc::AudioDeviceModule>   audioDeviceModule) {
		std::list<std::string> audioList;
		if (std::regex_match("audioplay://", publishFilter))
		{
			int16_t num_audioDevices = audioDeviceModule->PlayoutDevices();
			SPDLOG_LOGGER_INFO(spdlogptr, "nb audio devices: {}", num_audioDevices);

			for (int i = 0; i < num_audioDevices; ++i)
			{
				char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
				char id[webrtc::kAdmMaxGuidSize] = { 0 };
				if (audioDeviceModule->PlayoutDeviceName(i, name, id) != -1)
				{
					SPDLOG_LOGGER_INFO(spdlogptr, "audio device name:{}   id:{} ", name, id);
					std::string devname;
					devname = "audioplay://";
					devname += std::to_string(i);
					audioList.push_back(devname);
				}
			}
		}
		return audioList;
	}

	static rtc::scoped_refptr<webrtc::AudioSourceInterface> CreateAudioSource(const std::string& audiourl,
		const std::map<std::string, std::string>& opts,
		const std::regex& publishFilter,
		rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory,
		rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioDecoderfactory,
		rtc::scoped_refptr<webrtc::AudioDeviceModule>   audioDeviceModule) {
		rtc::scoped_refptr<webrtc::AudioSourceInterface> audioSource;

		rtc::scoped_refptr<AudioTrackSourceInput> audioSource1;
		audioDeviceModule->Terminate();
		audioSource1 = AudioTrackSourceInput::Create(audiourl, opts);
		return audioSource1;
		if ((audiourl.find("rtsp://") == 0) && (std::regex_match("rtsp://", publishFilter)))
		{
#ifdef HAVE_LIVE555
			audioDeviceModule->Terminate();
			audioSource = RTSPAudioSource::Create(audioDecoderfactory, audiourl, opts);
#endif
		/*	rtc::scoped_refptr<AudioTrackSourceInput> audioSource;
			audioDeviceModule->Terminate();
			audioSource = AudioTrackSourceInput::Create(audiourl, opts);
			return audioSource;*/


			/*int16_t num_audioDevices = audioDeviceModule->PlayoutDevices();
			int16_t idx_audioDevice = -1;
			char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
			char id[webrtc::kAdmMaxGuidSize] = { 0 };
			for (int i = 0; i < num_audioDevices; ++i)
			{
				if (audioDeviceModule->PlayoutDeviceName(i, name, id) != -1)
				{
					std::map<std::string, std::string> opts1;
					opts1["device_id"] = id;
					audioSource = AudioTrackSourceInput::Create(name, opts1);
					break;
				}
			}*/

		}
		else if ((audiourl.find("file://") == 0) && (std::regex_match("file://", publishFilter)))
		{
#ifdef HAVE_LIVE555
			audioDeviceModule->Terminate();
			audioSource = FileAudioSource::Create(audioDecoderfactory, audiourl, opts);
			//audioSource = FFmpegAudioSource::Create(audioDecoderfactory, audiourl, opts);
#endif
		}
		else if (std::regex_match("audiocap://", publishFilter))
		{
			audioDeviceModule->Init();
			int16_t num_audioDevices = audioDeviceModule->RecordingDevices();
			int16_t idx_audioDevice = -1;
			char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
			char id[webrtc::kAdmMaxGuidSize] = { 0 };
			if (audiourl.find("audiocap://") == 0) {
				int deviceNumber = atoi(audiourl.substr(strlen("audiocap://")).c_str());
				SPDLOG_LOGGER_INFO(spdlogptr, "audiourl: {}   device number:{} ", audiourl, deviceNumber);
				if (audioDeviceModule->RecordingDeviceName(deviceNumber, name, id) != -1)
				{
					idx_audioDevice = deviceNumber;
				}

			}
			else {
				for (int i = 0; i < num_audioDevices; ++i)
				{
					if (audioDeviceModule->RecordingDeviceName(i, name, id) != -1)
					{
						SPDLOG_LOGGER_INFO(spdlogptr, "audiourl: {}  idx_audioDevice:{}  name :{}", audiourl, i, name);
#ifdef WEBRTC_WIN			
						if (audiourl == GBK::Utf8ToGbk(name) || audiourl == name || GBK::Utf8ToGbk(audiourl.c_str()) == GBK::Utf8ToGbk(name))
						{
							idx_audioDevice = i;
							break;
						}
#endif
						if (audiourl == name)
						{
							idx_audioDevice = i;
							break;
						}
					}
				}
			}
			SPDLOG_LOGGER_ERROR(spdlogptr, "audiourl:{}  idx_audioDevice:{} num_audioDevices:{} ", audiourl, idx_audioDevice, num_audioDevices);
			if ((idx_audioDevice >= 0) && (idx_audioDevice < num_audioDevices))
			{
				audioDeviceModule->SetRecordingDevice(idx_audioDevice);
				audioDeviceModule->EnableBuiltInAEC(false);
				audioDeviceModule->EnableBuiltInAGC(false);
				audioDeviceModule->EnableBuiltInNS(false);
				audioDeviceModule->SetMicrophoneVolume(255);
				cricket::AudioOptions opt;
				audioSource = peer_connection_factory->CreateAudioSource(opt);
			}
		}
		return audioSource;
	}
};
