#include "VideoCaptureImpl.h"


#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
VideoCaptureImpl::VideoCaptureImpl()
{


}

VideoCaptureImpl::~VideoCaptureImpl()
{
	Destroy();
}

bool VideoCaptureImpl::Start()
{
	m_StartTime = 0;
	m_nFrameCount = 0;
	if (!m_videoCapturer)
	{
		for (int i = 0; i < 15; i++)
		{
			m_videoCapturer = OpenVideoCaptureDevice();
			if (m_videoCapturer)
			{
				break;
			}
			//很重要  有些摄像头 就必须等待一会才可以正常使用 不然会花屏
			rtc::Thread::SleepMs(300);
		}

		if (m_videoCapturer)
		{
			m_videoCapturer->CaptureStarted();
			return true;
		}
		else
		{
			RTC_LOG(LS_ERROR) << "开启摄像头失败";
		
			return false;
		}
	}
	return true;

}

void VideoCaptureImpl::Destroy()
{
	m_YuvCallbackList.clear();

	Stop(NULL);
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
m_FrameCallbackList.clear();
#endif // WEBRTC_WIN
	

}

void VideoCaptureImpl::Stop(VideoYuvCallBack yuvCallback)
{
	bool bEmpty = false;	
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
		while (it != m_YuvCallbackList.end())
		{
			if (it->target<void*>() == yuvCallback.target<void*>())
			{
				m_YuvCallbackList.erase(it);
				break;
			}
			it++;
		}

	}
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)

	if (m_FrameCallbackList.empty() )
	{
		bEmpty = true;
	}
#endif // WEBRTC_WIN
	
	if (m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}

	if (bEmpty)
	{
		if (m_videoCapturer)
		{
			m_videoCapturer->StopCapture();
			m_videoCapturer->DeRegisterCaptureDataCallback();
			// Release reference to VCM.
			m_videoCapturer = nullptr;
		}	
	}
}

rtc::scoped_refptr<webrtc::VideoCaptureModule> VideoCaptureImpl::OpenVideoCaptureDevice()
{
	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info_;
	device_info_.reset(webrtc::VideoCaptureFactory::CreateDeviceInfo());
	if (!device_info_)
	{
		return nullptr;
	}

	int num_devices = device_info_->NumberOfDevices();
	if (num_devices == 0)
	{
		return nullptr;
	}
	char device_name[256];
	char unique_name[256];
	bool bfind = false;

	for (int device = 0; device < num_devices; ++device)
	{
		device_info_->GetDeviceName(device, device_name, 256,
			unique_name, 256);
		if (std::string(device_name) == m_device_name)
		{
			bfind = true;
			break;
		}
	}

	if (!bfind)
	{
		return nullptr;
	}

	m_videoCapturer = webrtc::VideoCaptureFactory::Create(unique_name);
	if (m_videoCapturer.get() == NULL)
		return nullptr;

	m_videoCapturer->RegisterCaptureDataCallback(this);
	rtc::Thread::SleepMs(300);

	webrtc::VideoCaptureCapability capability;
	capability.width = static_cast<int32_t>(m_nWidth);
	capability.height = static_cast<int32_t>(m_nHeight);
	capability.maxFPS = static_cast<int32_t>(m_nFrameRate);
	capability.videoType = webrtc::VideoType::kI420;

	if (device_info_->GetBestMatchedCapability(m_videoCapturer->CurrentDeviceName(), capability, capability) < 0)
	{
		device_info_->GetCapability(m_videoCapturer->CurrentDeviceName(), 0, capability);
	}

	m_videoCapturer->StartCapture(capability);

	return m_videoCapturer;
}

void VideoCaptureImpl::Init(const char* devicename, int nWidth, int nHeight, int nFrameRate)
{
	m_device_name = devicename;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFrameRate = nFrameRate;
}

void VideoCaptureImpl::Init(std::map<std::string, std::string> opts)
{
	if (opts.find("devicename") != opts.end())
	{
		m_device_name = opts.at("devicename");
	}
	if (opts.find("width") != opts.end())
	{
		m_nWidth = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		m_nHeight = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		m_nFrameRate = std::stoi(opts.at("fps"));
	}
}

void VideoCaptureImpl::OnFrame(const webrtc::VideoFrame& video_frame)
{
	std::list<VideoYuvCallBack> yuvlist;
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	std::list<FrameCallBack> framelist;
#endif // WEBRTC_WIN
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		yuvlist = m_YuvCallbackList;
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
		framelist = m_FrameCallbackList;
		for (auto callbackfuc : framelist)
		{
			callbackfuc(video_frame);
		}
#endif // WEBRTC_WIN
	
	}


	rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
		video_frame.video_frame_buffer()->ToI420());

	for(auto callback : yuvlist)
	{
		if (callback)
		{
			if (m_StartTime == 0)
			{
				m_StartTime = rtc::TimeMillis();
			}
			callback((uint8_t*)buffer->DataY(), buffer->StrideY(),
				(uint8_t*)buffer->DataU(), buffer->StrideU(),
				(uint8_t*)buffer->DataV(), buffer->StrideV(),
				buffer->width(), buffer->height(), rtc::TimeMillis());
			m_nFrameCount++;
		}

	}
}
void VideoCaptureImpl::RegisterCallback(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			return;
		}
		it++;
	}
	m_YuvCallbackList.push_back(yuvCallback);

}

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)

void VideoCaptureImpl::StopFrameCallback(FrameCallBack frameCallback)
{
	bool bEmpty = false;
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		std::list<FrameCallBack>::iterator it = m_FrameCallbackList.begin();
		while (it != m_FrameCallbackList.end())
		{
			if (it->target<void*>() == frameCallback.target<void*>())
			{
				m_FrameCallbackList.erase(it);
				break;
			}
			it++;
		}
	}

	if (m_FrameCallbackList.empty() && m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}

	if (bEmpty)
	{
		if (m_videoCapturer)
		{
			m_videoCapturer->StopCapture();
			m_videoCapturer->DeRegisterCaptureDataCallback();
			// Release reference to VCM.
			m_videoCapturer = nullptr;
		}
	}
}
void VideoCaptureImpl::RegisterFrameCallback(FrameCallBack frameCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	for (auto it:m_FrameCallbackList)
	{
		if (it.target<void*>() == frameCallback.target<void*>())
		{
			return;
		}
	}
	m_FrameCallbackList.push_back(frameCallback);
}
#endif // WEBRTC_WIN



#if defined(WEBRTC_WIN) 
MyDesktopCapture::MyDesktopCapture()
{
	m_bStop.store(false);
}

MyDesktopCapture::~MyDesktopCapture()
{
	Destroy();
}

void MyDesktopCapture::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> desktopframe)
{
	std::list<VideoYuvCallBack> yuvlist;
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	std::list<FrameCallBack> framelist;
#endif // WEBRTC_WIN
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		
		yuvlist = m_YuvCallbackList;
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
		framelist = m_FrameCallbackList;
#endif // WEBRTC_WIN
	

	}




	if (result != webrtc::DesktopCapturer::Result::SUCCESS)
	{
		RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult capture error:" << (int)result;
		return;
	}
	
	int width = desktopframe->rect().width();
	int height = desktopframe->rect().height();

	rtc::scoped_refptr<webrtc::I420Buffer> I420buffer = webrtc::I420Buffer::Create(width, height);
	int stride = width;
	//uint8_t* yplane = buffer->MutableDataY();
	//uint8_t* uplane = buffer->MutableDataU();
	//uint8_t* vplane = buffer->MutableDataV();
	const int conversionResult = libyuv::ConvertToI420(desktopframe->data(), desktopframe->stride() * webrtc::DesktopFrame::kBytesPerPixel,
		I420buffer->MutableDataY(), I420buffer->StrideY(),
		I420buffer->MutableDataU(), I420buffer->StrideU(),
		I420buffer->MutableDataV(), I420buffer->StrideV(),
		0, 0,
		width, height,
		width, height,
		libyuv::kRotate0, libyuv::FOURCC_ARGB);
	if (conversionResult >= 0)
	{
		webrtc::VideoFrame videoFrame(I420buffer, webrtc::VideoRotation::kVideoRotation_0, rtc::TimeMicros());
		webrtc::VideoFrame frame=videoFrame;
		if ((m_nWidth == 0) && (m_nHeight == 0))
		{
			frame = videoFrame;

		}
		else
		{
			int height = m_nHeight;
			int width = m_nWidth;
			if (height == 0) {
				height = (videoFrame.height() * width) / videoFrame.width();
			}
			else if (width == 0) {
				width = (videoFrame.width() * height) / videoFrame.height();
			}
			int stride_y = width;
			int stride_uv = (width + 1) / 2;
			rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(width, height, stride_y, stride_uv, stride_uv);
			scaled_buffer->ScaleFrom(*videoFrame.video_frame_buffer()->ToI420());
			frame = webrtc::VideoFrame(scaled_buffer, webrtc::kVideoRotation_0, rtc::TimeMicros());

		}

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
		for (auto callbackfuc : framelist)
		{
			callbackfuc(frame);
		}
#endif // WEBRTC_WIN

	
		for (auto callback : yuvlist)
		{
			if (callback)
			{
				callback((uint8_t*)I420buffer->DataY(), I420buffer->StrideY(),
					(uint8_t*)I420buffer->DataU(), I420buffer->StrideU(),
					(uint8_t*)I420buffer->DataV(), I420buffer->StrideV(),
					I420buffer->width(), I420buffer->height(), rtc::TimeMillis());
			}
		}	
	}
	else {
		RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult conversion error:" << conversionResult;
	}	
}

void MyDesktopCapture::CaptureThread()
{
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run start";
	while (!m_bStop.load())
	{
		rtc::Thread::SleepMs(5);
		if (m_capturer)
		{
			m_capturer->CaptureFrame();
		}
	}
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run exit";
}

bool MyDesktopCapture::Start()
{
	webrtc::DesktopCaptureOptions options(webrtc::DesktopCaptureOptions::CreateDefault());
#if defined(WEBRTC_WIN) 
	options.set_allow_directx_capturer(true);
#endif
	m_capturer = webrtc::DesktopCapturer::CreateScreenCapturer(options);
	webrtc::DesktopCapturer::SourceList sourceList;
	m_capturer->GetSourceList(&sourceList);
	if (sourceList.size() > 0)
	{
		m_capturer->SelectSource(sourceList[0].id);
	}
	m_bStop.store(false);

	m_capturer->Start(this);
	m_capturer->CaptureFrame();

	if (!m_capturethread)
	{
		m_capturethread = (rtc::Thread::Create());
		m_capturethread->Start();
		m_capturethread->PostTask(std::bind(&MyDesktopCapture::CaptureThread, this));
	}

	return true;
}

void MyDesktopCapture::Destroy()
{
	m_YuvCallbackList.clear();

	Stop(NULL);
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	m_FrameCallbackList.clear();
	StopFrameCallback(NULL);
#endif // WEBRTC_WIN
}

void MyDesktopCapture::Stop(VideoYuvCallBack yuvCallback)
{
	m_bStop.store(true);
	bool bEmpty = false;
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			m_YuvCallbackList.erase(it);
			break;
		}
		it++;
	}
#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
	if (m_FrameCallbackList.empty())
	{
		bEmpty = true;
	}
#endif // WEBRTC_WIN
	if ( m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}
	if (bEmpty)
	{
		m_bStop.store(true);
		rtc::Thread::SleepMs(10);
		m_capturer.reset();
	}
}

void MyDesktopCapture::Init(const char* devicename, int nWidth, int nHeight, int nFrameRate)
{
	m_device_name = devicename;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFrameRate = nFrameRate;
}

void MyDesktopCapture::Init(std::map<std::string, std::string> opts)
{
	if (opts.find("devicename") != opts.end())
	{
		m_device_name = opts.at("devicename");
	}
	if (opts.find("width") != opts.end()) 
	{
		m_nWidth = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		m_nHeight = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		m_nFrameRate = std::stoi(opts.at("fps"));
	}
}

#if defined(WEBRTC_WIN) || defined(WEBRTC_LINUX)
void MyDesktopCapture::RegisterCallback(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			return;
		}
		it++;
	}
	m_YuvCallbackList.push_back(yuvCallback);

}

void MyDesktopCapture::StopFrameCallback(FrameCallBack frameCallback)
{
	bool bEmpty = false;
	std::lock_guard<std::mutex> _lock(m_mutex);

	std::list<FrameCallBack>::iterator it = m_FrameCallbackList.begin();
	while (it != m_FrameCallbackList.end())
	{
		if (it->target<void*>() == frameCallback.target<void*>())
		{
			m_FrameCallbackList.erase(it);
			break;
		}
		it++;
	}
	if (m_FrameCallbackList.empty() && m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}
	if (bEmpty)
	{
		m_bStop.store(true);
		rtc::Thread::SleepMs(10);
		m_capturer.reset();
	}

}

void MyDesktopCapture::RegisterFrameCallback(FrameCallBack frameCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	for (auto it : m_FrameCallbackList)
	{
		if (it.target<void*>() == frameCallback.target<void*>())
		{
			return;
		}
	}
	m_FrameCallbackList.push_back(frameCallback);
	
}
#endif // WEBRTC_WIN


#endif // WEBRTC_WIN

RtspVideoCapture::RtspVideoCapture(const std::string& uri, const std::map<std::string, std::string>& opts)
{
	m_bStop.store(false);

}

RtspVideoCapture::~RtspVideoCapture()
{
	Destroy();
}

bool RtspVideoCapture::Start()
{
	RTC_LOG(LS_INFO) << "LiveVideoSource::Start";
	return true;
}

void RtspVideoCapture::Destroy()
{
	RTC_LOG(LS_INFO) << "LiveVideoSource::stop";
	m_bStop.store(true);
	m_YuvCallbackList.clear();
	m_h264Callback = nullptr;
	Stop(NULL);
}

void RtspVideoCapture::Stop(VideoYuvCallBack yuvCallback)
{
	bool bEmpty = false;

	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			m_YuvCallbackList.erase(it);
			break;
		}
		it++;
	}
	if (m_YuvCallbackList.empty())
	{
		bEmpty = true;
	}

	if (bEmpty)
	{
	}
}

void RtspVideoCapture::Init(const char* devicename, int nWidth, int nHeight, int nFrameRate)
{

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFrameRate = nFrameRate;
}

void RtspVideoCapture::Init(std::map<std::string, std::string> opts)
{
	if (opts.find("width") != opts.end())
	{
		m_nWidth = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		m_nHeight = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		m_nFrameRate = std::stoi(opts.at("fps"));
	}
}

void RtspVideoCapture::RegisterCallback(VideoYuvCallBack yuvCallback)
{
	std::lock_guard<std::mutex> _lock(m_mutex);
	std::list<VideoYuvCallBack>::iterator it = m_YuvCallbackList.begin();
	while (it != m_YuvCallbackList.end())
	{
		if (it->target<void*>() == yuvCallback.target<void*>())
		{
			return;
		}
		it++;
	}
	m_YuvCallbackList.push_back(yuvCallback);
}

void RtspVideoCapture::CaptureThread()
{

}

bool RtspVideoCapture::onData(const char* id, unsigned char* buffer, int size, int64_t ts)
{
	if (m_bStop.load())
	{
		return false;
	}
	if (m_h264Callback)
	{
		m_h264Callback((char*)buffer, size, 1, m_nWidth, m_nHeight, m_nFrameRate, ts);
	}
	return true;
}
