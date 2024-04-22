
/*!
 * \file FFmpegDecoderAPI.h
 * \date 2021/09/02 15:48
 *
 * \author admin
 * Contact: user@company.com
 *
 * \brief
 *
 * TODO: long description
 *
 * \note
*/
#pragma once

#include "ffmpegTypes.h"
#include <string.h>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <queue>
#include <iostream>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

typedef std::function<void(uint8_t* y, int strideY, uint8_t* u, int strideU, uint8_t* v, int strideV, int nWidth, int nHeight, int64_t nTimeStamp)> FrameCallBackFunc;

typedef std::function<void(uint8_t* yuv, int nWidth, int nHeight, int strideY, int strideU, int strideV, bool bKey, int64_t nTimeStamp)> LocalYuvCallBackFunc;

typedef std::function<void(unsigned char* aPcm, int size, int64_t nTimeStamp)> LocalPcmCallBackFunc;

// 回调函数类型
using DecodedFrameCallback = std::function<void(uint8_t* buffer, int bufferSize, int fmt, int width, int height)>;




// 媒体帧信息定义
class FrameInfo
{
public:
	
	FrameInfo() {
	};
	~FrameInfo() {};

public:
	// 视频编码格式
	FFmpeg_CodecType VCodec= CAREYE_CODEC_NONE;
	// 音频解码格式，无音频则置为CAREYE_CODEC_NONE
	FFmpeg_CodecType ACodec= CAREYE_CODEC_NONE;
	//编解码格式 
	DecodeType    DecType= DecodeType::kDecodeSoft;
	// 视频帧率(FPS)
	unsigned char	FramesPerSecond;
	// 视频宽度像素
	unsigned short	Width=0;
	// 视频的高度像素
	unsigned short  Height=0;
	// 视频码率，越高视频越清楚，相应体积也越大 如：4000000
	unsigned int	VideoBitrate=0;
	// 音频采样率
	unsigned int	SampleRate=0;
	// 音频声道数
	unsigned int	Channels=0;
	// 音频采样精度 16位 8位等，库内部固定为16位
	unsigned int	BitsPerSample=16;
	// 音频比特率 如：64000，越高声音越清楚，相应体积也越大
	unsigned int	AudioBitrate=0;

	unsigned int    bits_per_coded_sample;




};




class  FFmpegDecoder;

namespace H265
{
	enum NaluType : uint8_t {
		kIdr = 19,
		kSps = 33,
		kPps = 34,
		kSei = 39,
	};

	// Get the NAL type from the header byte immediately following start sequence.
	 NaluType   ParseNaluType(uint8_t data);

}




class  FFmpegDecoderAPI
{
public:
	class Frame
	{
	public:
		Frame() {

		};
		Frame(unsigned char* pBytes, int nSize, bool bKey, int64_t timestamp_ms) :
			m_nSize(nSize),
			m_timestamp_ms(timestamp_ms) {
			m_pBytes = static_cast<unsigned char*>(malloc(nSize));
			memcpy(m_pBytes, pBytes, m_nSize);
			m_bKey = bKey;

		}
		~Frame()
		{
			free(m_pBytes);
		};

		unsigned char* m_pBytes = nullptr;
		int64_t               m_timestamp_ms = 0;
		int					   m_nSize = 0;
		bool m_bKey = false;
	};


public:
	FFmpegDecoderAPI() {};

	FFmpegDecoderAPI(const std::map<std::string, std::string>& opts, bool wait)
	{
	}

	virtual ~FFmpegDecoderAPI()
	{
	}

	static FFmpegDecoderAPI* CreateDecoder(std::map<std::string, std::string> opts = {});


	virtual void Start()=0;

	/*
	* Comments: 创建一个解码器对象
	* Param aInfo: 要解码的媒体信息
	* @Return 成功返回true，否则返回NULL
	*/
	virtual bool createDecoder(FrameInfo pFrameInfo)=0;

	virtual bool createDecoder(char* szCodecName, int nWidth, int nHeight)=0;

	/*
	* Comments: 释放解码器资源
	* Param aDecoder: 要释放的解码器
	* @Return None
	*/
	virtual void stopDecode()=0;

	virtual bool hasDecoder()=0;

	virtual int getYUVSize()=0;

	virtual bool CaptureJpegFromAVFrame(char* OutputFileName, int quality)=0;

	virtual void PostFrame(unsigned char* aBytes, int aSize, bool bKey, uint64_t ts) = 0;

	/*
	* Comments: 将输入视频解码为YUV420格式数据输出
	* Param aBytes: 要进行解码的视频流
	* Param aSize: 要解码视频流字节数
	* Param aYuv: [输出] 解码成功后输出的YUV420数据
	* @Return int < 0解码失败，> 0为解码后YUV420的字节个数 ==0表示参数无效
	*/
	virtual int DecoderYUV420(unsigned char* aBytes, int aSize, unsigned char* aYuv)=0;

	/*
* Comments: 将输入音频解码为PCM格式数据输出
* Param aDecoder: 申请到的有效解码器
* Param aBytes: 要进行解码的音频流
* Param aSize: 要解码音频流字节数
* Param aYuv: [输出] 解码成功后输出的PCM数据
* @Return int < 0解码失败，> 0为解码后PCM的字节个数 ==0表示参数无效
*/
	virtual int DecoderPCM(unsigned char* aBytes, int aSize, unsigned char* aPcm, int& nb_samples)=0;

	virtual bool FFMPEGGetWidthHeight(unsigned char* videooutdata, int videooutdatasize, char* videoName, int* outwidth, int* outheight)=0;
	// 注册回调
	void RegisterDecodeCallback(FrameCallBackFunc callbackfuc)
	{
		m_callbackfuc = callbackfuc;
	};

	// 注册回调
	void RegisterDecodeCallback(LocalYuvCallBackFunc callbackfuc)
	{
		m_yuvcallbackfuc = callbackfuc;
	};
	// 注册回调
	void RegisterDecodeCallback(LocalPcmCallBackFunc callbackfuc)
	{
		m_pcmcallbackfuc = callbackfuc;
	};
	// 注册回调
	void RegisterDecodeCallback(DecodedFrameCallback callbackfuc)
	{
		m_DecodedFrameCallback = callbackfuc;
	};


	FrameCallBackFunc m_callbackfuc = nullptr; //本地播放回调

	LocalYuvCallBackFunc   m_yuvcallbackfuc = nullptr;//本地播放回调

	LocalPcmCallBackFunc    m_pcmcallbackfuc = nullptr;

	// 全局回调函数对象
	DecodedFrameCallback m_DecodedFrameCallback=nullptr;
	//unsigned char* m_pPcm = NULL;


};

class  AVPacket;
class AVFrame;
class  FFmpegAudioDecoderAPI
{
#define AV_NUM_DATA_POINTERS 8
public:
	FFmpegAudioDecoderAPI() {};
	virtual ~FFmpegAudioDecoderAPI() {};

	static FFmpegAudioDecoderAPI* CreateDecoder(std::map<std::string, std::string> opts = {});

	
	virtual bool PushAudioPacket( uint8_t* aBytes, int aSize, int64_t ts) = 0;
	
	virtual bool PushAudioPacket( AVPacket* packet, AVFrame* frame) = 0;


	virtual bool DecodeData(uint8_t* inData, int inSize, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)=0;

	virtual bool DecodeData(AVPacket* packet, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)=0;

	virtual void RegisterDecodeCallback(std::function <void(uint8_t** data, int size)> result_callback)=0;
	virtual void RegisterDecodeCallback( std::function <void(AVFrame* data)> result_callback)=0;
	virtual    AVFrame* frame_alloc()=0;
	virtual bool DecodeData(uint8_t* inData, int inSize)=0;
};