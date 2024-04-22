

#pragma once
#include "VideoCapture.h"
#include <string>
#include <list>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

#include "libyuv.h"
#include "rtc_base/thread.h"
#include "media/base//adapted_video_track_source.h"
#include "pc/video_track_source.h"
#include "api/video/i420_buffer.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"



class RtspVideoCapture :public VideoCapture
{


public:
	RtspVideoCapture(const std::string& uri, const std::map<std::string, std::string>& opts = {});

	~RtspVideoCapture();


	/*VideoCapture callback*/
	virtual bool Start();

	virtual void Destroy();

	virtual void Stop(VideoYuvCallBack yuvCallback);

	virtual void Init(const char* devicename, int nWidth, int nHeight, int nFrameRate);

	virtual void Init(const char* strJson)
	{};

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback);

	virtual void RegisterH264Callback(H264CallBack h264Callback) { m_h264Callback = h264Callback; };

	


	virtual bool onData(const char* id, unsigned char* buffer, int size, int64_t ts);

	virtual bool onData(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp) { return true; };
	
private:
	void CaptureThread();
	void Init(std::map<std::string, std::string> opts = {});

private:
	char        m_stop;

private:

	int m_nWidth = 1280;
	int m_nHeight = 720;
	int m_nFrameRate = 30;


	int64_t next_timestamp_us_ = rtc::kNumMicrosecsPerMillisec;
	std::atomic<bool>m_bStop;
	std::mutex m_mutex;                //互斥锁		


	std::list<VideoYuvCallBack> m_YuvCallbackList;

	H264CallBack m_h264Callback;
	std::thread                        m_capturethread;
	cricket::VideoFormat               m_format;
	std::vector<uint8_t>               m_cfg;
	std::map<std::string, std::string> m_codec;



};



















typedef std::function<void(const webrtc::VideoFrame& videoFrame)> FrameCallBack;


//摄像头数据获取
class VideoCaptureImpl :public VideoCapture, public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
	VideoCaptureImpl();
	~VideoCaptureImpl();

	/*VideoCapture callback*/
	virtual bool Start();

	virtual void Destroy() ;

	virtual void Stop(VideoYuvCallBack yuvCallback);

	virtual void Init(const char* devicename, int nWidth, int nHeight, int nFrameRate);

	virtual void Init(std::map<std::string, std::string> opts = {});

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback);

	virtual void RegisterH264Callback(H264CallBack yuvCallback) {};

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	virtual void RegisterFrameCallback(FrameCallBack frameCallback);
	virtual void StopFrameCallback(FrameCallBack frameCallback);

#endif // WEBRTC_WIN

	//rtc::VideoSinkInterface<webrtc::VideoFrame>
	virtual void OnFrame(const webrtc::VideoFrame& video_frame);

	//打开设备
	rtc::scoped_refptr<webrtc::VideoCaptureModule> OpenVideoCaptureDevice();
private:

	rtc::scoped_refptr<webrtc::VideoCaptureModule> m_videoCapturer;

	std::string m_device_name;
	int m_nWidth = 1920;
	int m_nHeight = 1080;
	int m_nFrameRate = 30;
	webrtc::VideoType m_nVideoType = webrtc::VideoType::kI420;

	std::vector<std::string> m_vec_devicename;
	std::mutex m_mutex;
	std::list<VideoYuvCallBack> m_YuvCallbackList;


	int64_t m_StartTime = 0;
	int64_t m_nFrameCount = 0;

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	std::list<FrameCallBack> m_FrameCallbackList;
#endif // WEBRTC_WIN


};

#if defined(WEBRTC_WIN) 
//桌面设局获取
class MyDesktopCapture : public VideoCapture, public webrtc::DesktopCapturer::Callback
{
public:
	explicit MyDesktopCapture();

	~MyDesktopCapture();

	// overide webrtc::DesktopCapturer::Callback
	void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> desktopframe) override;

	void CaptureThread();


	/*VideoCapture callback*/
	virtual bool Start();

	virtual void Destroy();

	virtual void Stop(VideoYuvCallBack yuvCallback);



	virtual void Init(const char* devicename, int nWidth, int nHeight, int nFrameRate);

	virtual void Init(std::map<std::string, std::string> opts = {});

	virtual void RegisterCallback(VideoYuvCallBack yuvCallback);

	virtual void RegisterH264Callback(H264CallBack yuvCallback) {};
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	virtual void StopFrameCallback(FrameCallBack frameCallback);
	virtual void RegisterFrameCallback(FrameCallBack frameCallback);
#endif // WEBRTC_WIN


private:
	std::unique_ptr<webrtc::DesktopCapturer> m_capturer;

	std::string m_device_name;
	int m_nWidth=1920;
	int m_nHeight=1080;
	int m_nFrameRate=30;
	webrtc::VideoType m_nVideoType = webrtc::VideoType::kI420;


	std::shared_ptr<rtc::Thread> m_capturethread;
	int64_t next_timestamp_us_ = rtc::kNumMicrosecsPerMillisec;
	std::atomic<bool>m_bStop;
	std::mutex m_mutex;                //互斥锁		
	std::string m_desktopname;

	std::list<VideoYuvCallBack> m_YuvCallbackList;

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	std::list<FrameCallBack> m_FrameCallbackList;
#endif // WEBRTC_WIN

};

#endif // WEBRTC_WIN