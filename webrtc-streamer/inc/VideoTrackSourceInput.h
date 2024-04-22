#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "../capture/VideoCapture.h"

#include "rtc_base/thread.h"
#include "media/base//adapted_video_track_source.h"
#include "pc/video_track_source.h"
#include "api/video/i420_buffer.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"

#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"


#include "MediaFifo.h"
class VideoTrackSourceInput : public rtc::AdaptedVideoTrackSource
{
public:
	VideoTrackSourceInput();
	~VideoTrackSourceInput();

	static VideoTrackSourceInput* Create(const std::string& videourl, const std::map<std::string, std::string>& opts);

	bool Init(size_t width,
		size_t height,
		size_t target_fps,
		const std::string& videourl);

	bool Init(std::string videourl, std::map<std::string, std::string> opts);

	//修改输入源
	void changeVideoInput(size_t width,
		size_t height,
		size_t target_fps,
		std::string videourl);

	// AdaptedVideoTrackSource implementation.	
	bool is_screencast() const override;

	absl::optional<bool> needs_denoising() const override;

	webrtc::MediaSourceInterface::SourceState state() const override;

	bool remote() const override;

	void InputVideoFrame(const unsigned char* y, const unsigned char* u, const unsigned char* v,
		int width, int height, int frame_rate);

	void InputVideoFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp);

	bool InputVideoFrame(unsigned char* data, size_t size, int nWidth, int nHeigh, int fps);

	//	//直接发送h264的数据
	bool InputVideoFrame(const char* id, unsigned char* buffer, size_t size, int nWidth, int nHeigh, int64_t ts);

	void Run();
private:
	VideoCapture* m_vCapture = nullptr;
	std::string m_videourl;
	std::map<std::string, std::string> m_opts;
	std::mutex m_mutex;                                        //互斥锁	
	int64_t next_timestamp_us_ = rtc::kNumMicrosecsPerMillisec;
	int64_t                              m_prevts = 0;//上一帧的时间
	std::atomic<bool>m_bStop;
	CMediaFifo videoFifo;
	std::condition_variable m_condition;              // 条件变量，用于线程等待
	std::shared_ptr<std::thread>  m_thread;
	int m_nWidth, m_nHeigh, m_fps;
	size_t  m_fifosize = 0;
	
};
