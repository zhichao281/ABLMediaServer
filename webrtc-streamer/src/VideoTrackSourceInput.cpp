#include "VideoTrackSourceInput.h"

#include "EncodedVideoFrameBuffer.h"
#include "common_video/h264/h264_common.h"
#include "common_video/h264/sps_parser.h"
VideoTrackSourceInput::VideoTrackSourceInput() {

	m_bStop.store(false);
	videoFifo.InitFifo(1024 * 1024 * 3);
	Run();
}

VideoTrackSourceInput* VideoTrackSourceInput::Create(const std::string& videourl, const std::map<std::string, std::string>& opts)
{
	VideoTrackSourceInput* vcm_capturer = new  rtc::RefCountedObject<VideoTrackSourceInput>();
	size_t width = 0;
	size_t height = 0;
	size_t fps = 0;
	if (opts.find("width") != opts.end()) {
		width = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		height = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		fps = std::stoi(opts.at("fps"));
	}
	//if (!vcm_capturer->Init(width, height, fps, videourl)) {
	//	RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << width
	//		<< ", h = " << height << ", fps = " << fps
	//		<< ")";
	//	return nullptr;
	//}
	if (!vcm_capturer->Init(videourl, opts)) {
		RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << width
			<< ", h = " << height << ", fps = " << fps
			<< ")";
		return nullptr;
	}
	return vcm_capturer;
}


VideoTrackSourceInput::~VideoTrackSourceInput()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_bStop.store(true);
		m_vCapture->RegisterH264Callback(nullptr);
		m_condition.notify_all();

	}
	if (m_thread->joinable())
	{
		m_thread->join();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	videoFifo.FreeFifo();
	if (m_vCapture)
	{
		VideoCaptureManager::getInstance().RemoveInput(m_videourl);
		m_vCapture = nullptr;
	}
	printf("VideoTrackSourceInput end \r\n");
}
bool VideoTrackSourceInput::Init(size_t width, size_t height, size_t target_fps, const std::string& videourl)
{

	m_videourl = videourl;
	m_vCapture = VideoCaptureManager::getInstance().GetInput(videourl);
	m_vCapture->Init(videourl.c_str(), width, height, target_fps);
	m_vCapture->RegisterCallback([this](uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)
		{
			InputVideoFrame(y, strideY, u, strideU, v, strideV, nWidth, nHeight,
				nTimeStamp);
		});
	m_vCapture->RegisterH264Callback([this](char* h264_raw, int file_size, bool bKey, int nWidth, int nHeight, int fps, int64_t nTimeStamp)
		{
			m_nWidth = nWidth;
			m_nHeigh = nHeight;
			m_fps = fps;
			// 使用互斥锁保护共享资源
			std::unique_lock<std::mutex> lock(m_mutex);
			videoFifo.push((unsigned char*)h264_raw, file_size);
			m_fifosize = videoFifo.GetSize();
			m_condition.notify_one();

		});

	m_vCapture->Start();
	return true;

}
bool VideoTrackSourceInput::Init(std::string videourl, std::map<std::string, std::string> opts)
{
	m_videourl = videourl;
	m_opts = opts;
	size_t width = 0;
	size_t height = 0;
	size_t fps = 0;
	if (opts.find("width") != opts.end()) {
		width = std::stoi(opts.at("width"));
	}
	if (opts.find("height") != opts.end()) {
		height = std::stoi(opts.at("height"));
	}
	if (opts.find("fps") != opts.end()) {
		fps = std::stoi(opts.at("fps"));
	}
	/// <summary>
	/// 
	/// </summary>
	/// <param name="videourl"></param>
	/// <param name="opts"></param>
	/// <returns></returns>
	return Init(width, height, fps, videourl);
}
void VideoTrackSourceInput::changeVideoInput(size_t width, size_t height, size_t target_fps, std::string videourl)
{
	Init(width, height, target_fps, videourl);

}
webrtc::MediaSourceInterface::SourceState VideoTrackSourceInput::state() const
{
	return webrtc::MediaSourceInterface::kLive;
}

bool VideoTrackSourceInput::remote() const
{
	return false;
}

bool VideoTrackSourceInput::is_screencast() const
{
	if ((m_videourl.find("window://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("screen://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("rtsp://") == 0))
	{
		return true;
	}
	else if ((m_videourl.find("file://") == 0))
	{
		return true;
	}
	else
	{
		return false;
	}

}

absl::optional<bool> VideoTrackSourceInput::needs_denoising() const
{
	return false;
}

void VideoTrackSourceInput::InputVideoFrame(const unsigned char* y, const unsigned char* u, const unsigned char* v, int width, int height, int frame_rate)
{
	std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
	_worker_thread_ptr->Start();
	_worker_thread_ptr->PostTask([&]()
		{
			rtc::scoped_refptr<webrtc::I420Buffer> buffer(webrtc::I420Buffer::Copy(width, height, y, width, u, width / 2, v, width / 2));

			next_timestamp_us_ = rtc::TimeMicros();

			webrtc::VideoFrame frame = webrtc::VideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0, next_timestamp_us_);

			next_timestamp_us_ += rtc::kNumMicrosecsPerSec / frame_rate;

			OnFrame(frame);

		}
	);

}

void VideoTrackSourceInput::InputVideoFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)
{

	std::shared_ptr<rtc::Thread> _worker_thread_ptr(std::move(rtc::Thread::Create()));
	_worker_thread_ptr->Start();
	_worker_thread_ptr->PostTask([&]()
		{
			rtc::scoped_refptr<webrtc::I420Buffer> buffer(webrtc::I420Buffer::Copy(nWidth, nHeight, y, nWidth, u, nWidth / 2, v, nWidth / 2));

			next_timestamp_us_ = rtc::TimeMicros();

			webrtc::VideoFrame frame = webrtc::VideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0, next_timestamp_us_);


			OnFrame(frame);

		}
	);

}

bool VideoTrackSourceInput::InputVideoFrame(unsigned char* data, size_t size, int nWidth, int nHeigh, int fps)
{
	
	int videosize = videoFifo.GetSize();
	int start_offset = 0;
	webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
	std::vector<webrtc::H264::NaluIndex> naluIndexes = webrtc::H264::FindNaluIndices(data, size);
	for (webrtc::H264::NaluIndex index : naluIndexes) {
		webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(data[index.payload_start_offset]);
		if (nalu_type == webrtc::H264::NaluType::kIdr)
		{
			frameType = webrtc::VideoFrameType::kVideoFrameKey;
			break;
		}
		if (nalu_type == webrtc::H264::NaluType::kSps)
		{
			start_offset = index.start_offset;

		}

	}
	auto  timestamp_us_ = rtc::TimeMillis();
	int64_t perio = timestamp_us_ - m_prevts;
	m_prevts = rtc::TimeMillis();
	if (perio < 1)
	{
		perio = 1;
	}
	if (fps < 1)
	{
		fps = 30;
	}
	int64_t nsleeptime = 40;
	if (videosize <= 6)
	{
		nsleeptime = 1000 / fps - perio +8;
	}
	else if (videosize > 6 && videosize <= 13)
	{
		nsleeptime = 1000 / fps- perio;

	}
	else if (videosize > 13 && videosize <= 20)
	{
		nsleeptime = 1000 / fps - perio - 5;
	}
	else if (videosize > 20 && videosize <= 30)
	{
		nsleeptime = 1000 / fps - perio - 10;
	}
	else if (videosize > 30 && videosize <= 40)
	{
		nsleeptime = 1000 / fps - perio - 15;
	}
	else
		nsleeptime = 2;

	if (nsleeptime > 0 && nsleeptime<100)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(nsleeptime));
	}
	//printf("nsleeptime=[%ld] perio=[%ld] videosize=[%d] \r\n", nsleeptime, perio, videosize);
	if (m_bStop.load())
	{
		return false;
	}

	//rtc::scoped_refptr<webrtc::EncodedImageBuffer> imageframe = webrtc::EncodedImageBuffer::Create(data, size);
	rtc::scoped_refptr<webrtc::EncodedImageBuffer> imageframe = webrtc::EncodedImageBuffer::Create(data + start_offset, size - start_offset);
	
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(nWidth, nHeigh, imageframe, frameType);
	//	webrtc::VideoFrame frame(buffer, webrtc::kVideoRotation_0, next_timestamp_us_);
	//int64_t ts = rtc::TimeMillis();

	
	int64_t ts = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000 / 1000;
	webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
		.set_video_frame_buffer(buffer)
		.set_rotation(webrtc::kVideoRotation_0)
		.set_timestamp_ms(ts)
	    .set_ntp_time_ms(ts)
		.set_id(ts)
		.build();
	OnFrame(frame);
	return true;
}


bool VideoTrackSourceInput::InputVideoFrame(const char* id, unsigned char* buffer, size_t size, int nWidth, int nHeigh, int64_t ts)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	webrtc::VideoFrameType frameType = webrtc::VideoFrameType::kVideoFrameDelta;
	std::vector<webrtc::H264::NaluIndex> naluIndexes = webrtc::H264::FindNaluIndices(buffer, size);
	for (webrtc::H264::NaluIndex index : naluIndexes) {
		webrtc::H264::NaluType nalu_type = webrtc::H264::ParseNaluType(buffer[index.payload_start_offset]);
		if (nalu_type == webrtc::H264::NaluType::kIdr)
		{
			frameType = webrtc::VideoFrameType::kVideoFrameKey;
			break;
		}
	}

	rtc::scoped_refptr<webrtc::EncodedImageBuffer> imagebuffer = webrtc::EncodedImageBuffer::Create(buffer, size);
	next_timestamp_us_ = rtc::TimeMicros();
	rtc::scoped_refptr<webrtc::VideoFrameBuffer> framebuffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(nWidth, nHeigh, imagebuffer, frameType);
	webrtc::VideoFrame frame(framebuffer, webrtc::kVideoRotation_0, next_timestamp_us_);
	OnFrame(frame);
	//

	//rtc::scoped_refptr<webrtc::EncodedImageBuffer> image_frame = _custom_nvenc_encoder->Encode(video_frame);
	//rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = rtc::make_ref_counted<EncodedVideoFrameBuffer>(_width, _height, image_frame);
	//webrtc::VideoFrame final_frame(buffer, webrtc::kVideoRotation_0, _next_timestamp_us);
	//OnFrame(final_frame);


	return true;
}



void VideoTrackSourceInput::Run()
{
	if (m_thread == nullptr)
	{
		m_thread = std::make_shared<std::thread>([=]()
			{
				while (!m_bStop.load())
				{
					unsigned char* pData = nullptr;
					int            nLength;
					{
						// 使用互斥锁保护共享资源
						std::unique_lock<std::mutex> lock(m_mutex);
						// 等待条件变量，直到有数据可用			
						m_fifosize = videoFifo.GetSize();
						m_condition.wait(lock, [this] { return (m_fifosize >0 || m_bStop.load()); });
						if (m_bStop.load())
						{
							return;
						}						
						pData = videoFifo.pop(&nLength);		
					}
					if (pData != NULL && nLength > 0)
					{
						InputVideoFrame(pData, nLength, m_nWidth, m_nHeigh, m_fps);
						videoFifo.pop_front();
					}
				}
			});
	}
}
