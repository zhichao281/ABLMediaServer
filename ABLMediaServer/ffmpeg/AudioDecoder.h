#pragma once
#ifdef __cplusplus
extern"C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libavutil/fifo.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avstring.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#ifdef __cplusplus
};
#endif
#include <string>
#include <functional>
#include <map>


#include "FFmpegDecoderAPI.h"

class AudioDecoder :public FFmpegAudioDecoderAPI
{
public:
	AudioDecoder(std::string par, std::function <void(uint8_t** data,int size)> result_callback);

	AudioDecoder(const std::map<std::string, std::string>& opts);
	~AudioDecoder();


	

public:
	virtual bool PushAudioPacket( uint8_t* aBytes, int aSize, int64_t ts) override;

	virtual bool PushAudioPacket( AVPacket* packet, AVFrame* frame) override;

	virtual bool DecodeData(uint8_t* inData, int inSize, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)override;

	virtual bool DecodeData(uint8_t* inData, int inSize)override;

	virtual bool DecodeData(AVPacket* packet, uint8_t* outData[AV_NUM_DATA_POINTERS], int* outSize)override;

	virtual void RegisterDecodeCallback(std::function <void(uint8_t** data, int size)> result_callback) override;
	
	virtual void RegisterDecodeCallback(std::function <void(AVFrame* data)> result_callback )override;

	virtual    AVFrame* frame_alloc();


private:
	bool m_success;
	std::mutex              m_mutex;
	std::function <void(uint8_t** data, int size)> m_callback=nullptr;

	std::function <void(AVFrame* data)> m_Framecallback = nullptr;

	int m_nb_channels=2;

	int m_sample_rate=48000;

	AVCodecID m_codec_id;

	int m_bit_rate;

	AVCodecContext* m_codec_context;

	int m_format;

};

