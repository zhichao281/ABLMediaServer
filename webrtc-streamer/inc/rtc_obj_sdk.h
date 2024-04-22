/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once
#define WIN32_LEAN_AND_MEAN	
 // 添加要在此处预编译的标头

#include <map>
#include <list>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include "../capture/VideoCapture.h"
#include "../capture/AudioCapture.h"

typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> LocalFrameCallBackFunc;

typedef std::function<void(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight)> RemoteFrameCallBackFunc;





class WEBRTCSDK_EXPORTSIMPL MainWndCallback {
public:
	MainWndCallback()
	{

	}
	virtual ~MainWndCallback() {}

	

	//摄像头yuv数据回调
	virtual void onLocalFrame(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;
	
	//远程视频yuv数据回调
	virtual void onRemoteFrame(std::string userID, uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight) = 0;

	//本地麦克风数据回调
	virtual void onLocalAudio(const void* audio_data ,int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//对方麦克风数据回调
	virtual void onRemoteAudio(std::string userID, const void* audio_data, int sample_rate_hz, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) = 0;

	//远程房间停止发送视频流
	virtual  void onRemoveTrack(std::string userID) = 0;

	//websocket的消息 h回调
	virtual  void onWsMessage(std::string strMessage) = 0;

	//log消息回调
	virtual  void onLogMessage(std::string strLog) = 0;

	//错误信息回调
	virtual void onError(int nErrorCode, std::string msg) = 0;


	virtual void OnDataChannel(std::string userID, const char* data, uint32_t len, bool binary) = 0;
};

class PeerConnectionManager;
class HttpServerRequestHandler;

class WEBRTCSDK_EXPORTSIMPL WebRtcEndpoint {
public:
	//初始化
	void init(const char* webrtcConfig, std::function<void(const char* callbackJson, void* pUserHandle)> callback);
	
	//释放
	void Uninit();
	
	//主动关闭某一路播放
	bool stopWebRtcPlay(const char* peerid);
	
	// 主动关闭某一个媒体源 
	bool  deleteWebRtcSource(const char* szMediaSource);

	void createIceServers(std::string username, std::string realm,
		std::string externalIp, std::string listeningIp,
		int listeningPort, int minPort, int maxPort);

	void createIceServers(const char* callbackJson);

public:
	static WebRtcEndpoint& getInstance();
private:
	WebRtcEndpoint(); 

	~WebRtcEndpoint() = default;

	WebRtcEndpoint(const WebRtcEndpoint&) = delete;

	WebRtcEndpoint& operator=(const WebRtcEndpoint&) = delete;


	PeerConnectionManager* webRtcServer;

	HttpServerRequestHandler* httpServer;

	std::function<void(const char* callbackJson, void* pUserHandle)>  m_callback;

	std::atomic<bool> bInit;

	std::thread* m_turnThread;

};

