/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** screencapturer.cpp
**
** -------------------------------------------------------------------------*/

#ifdef USE_X11

#include "rtc_base/logging.h"

#include "desktopcapturer.h"

void DesktopCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) {
	
	RTC_LOG(LS_INFO) << "DesktopCapturer:OnCaptureResult";
	
	if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
		int width = 0, height = 0, newWidth = 0, newHeight = 0;
		int conversionResult = 0;
		rtc::scoped_refptr<webrtc::I420Buffer> I420buffer;
		if (m_strSource.find("window://") == 0)
		{
#if defined(WEBRTC_WIN)
			HWND window = reinterpret_cast<HWND>(m_id);
			LONG style = GetWindowLong(window, GWL_STYLE);
			int nFramewidth = 0;
			int bottom_height = 0;
			int visible_border_height = 0;
			int top_height = 0;
			if (style & WS_THICKFRAME || style & DS_MODALFRAME)
			{
				nFramewidth = GetSystemMetrics(SM_CXSIZEFRAME);
				bottom_height = GetSystemMetrics(SM_CYSIZEFRAME);
				visible_border_height = GetSystemMetrics(SM_CYBORDER);
				top_height = visible_border_height;
			}
			//width = frame->size().width();
			//height = frame->size().height();


	/*		width = frame->size().width() + nFramewidth + nFramewidth;
			height = frame->size().height() + bottom_height - top_height;*/
			width = frame->stride() / webrtc::DesktopFrame::kBytesPerPixel;
			height = frame->rect().height();

			newWidth = frame->size().width() - nFramewidth - nFramewidth;
			newHeight = frame->size().height() - bottom_height + top_height;
			if (newWidth % 8 != 0)
			{
				newWidth = (newWidth / 8 - 1) * 8;
			}
			if (newHeight % 8 != 0)
			{
				newHeight = (newHeight / 8 - 1) * 8;
			}
			int crop_x = (width - newWidth) / 2;
			int crop_y = (height - newHeight) / 2;


			I420buffer = webrtc::I420Buffer::Create(newWidth, newHeight);
			conversionResult = libyuv::ConvertToI420(frame->data(), 0, I420buffer->MutableDataY(),
				I420buffer->StrideY(), I420buffer->MutableDataU(),
				I420buffer->StrideU(), I420buffer->MutableDataV(),
				I420buffer->StrideV(), crop_x, crop_y, width, height, newWidth,
				newHeight, libyuv::kRotate0, libyuv::FOURCC_ARGB);
#endif
			

		}
		else
		{
			int width = frame->stride() / webrtc::DesktopFrame::kBytesPerPixel;
			int height = frame->rect().height();

			I420buffer = webrtc::I420Buffer::Create(width, height);

			conversionResult = libyuv::ConvertToI420(frame->data(), 0,
				I420buffer->MutableDataY(), I420buffer->StrideY(),
				I420buffer->MutableDataU(), I420buffer->StrideU(),
				I420buffer->MutableDataV(), I420buffer->StrideV(),
				0, 0,
				width, height,
				I420buffer->width(), I420buffer->height(),
				libyuv::kRotate0, ::libyuv::FOURCC_ARGB);
		}


								
				
		if (conversionResult >= 0) {
			webrtc::VideoFrame videoFrame(I420buffer, webrtc::VideoRotation::kVideoRotation_0, rtc::TimeMicros());
			if ( (m_height == 0) && (m_width == 0) ) {
				m_broadcaster.OnFrame(videoFrame);	

			} else {
				int height = m_height;
				int width = m_width;
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

	            webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(scaled_buffer)
					.set_rotation(webrtc::kVideoRotation_0)
					.set_timestamp_us(rtc::TimeMicros())
					.build();
				
						
				m_broadcaster.OnFrame(frame);
			}
		} else {
			RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult conversion error:" << conversionResult;
		}				

	} else {
		RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult capture error:" << (int)result;
	}
}
		
void DesktopCapturer::CaptureThread() {
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run start";
	while (IsRunning()) {

		if (m_id < 0)
		{
			m_isrunning = false;
			return;

		}
		if (m_capturer->SelectSource(m_id))
		{
			m_capturer->CaptureFrame();
			m_nError = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}
		else
		{
			m_nError++;
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			if (m_nError > 1000 * 60 * 5)
			{
				m_isrunning = false;
				return;
			}
			else
			{
				continue;
			}
		}


	}
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run exit";
}
bool DesktopCapturer::Start() {
	m_isrunning = true;
	m_capturethread = std::thread(&DesktopCapturer::CaptureThread, this); 
	m_capturer->Start(this);
	return true;
}
		
void DesktopCapturer::Stop() {
	m_isrunning = false;
	m_capturethread.join(); 
}
		
#endif

