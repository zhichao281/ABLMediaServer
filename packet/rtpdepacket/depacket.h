#ifndef _RTP_PACKET_DEPACKET_DEPACKET_H_
#define _RTP_PACKET_DEPACKET_DEPACKET_H_

#include <stdint.h>
#include <memory>
#include <unordered_map>
#include "rtp_depacket.h"

#define      MaxNaluBlockCount    32   //最大Nalu段

struct NaluHeardPosition
{
	int nPos;        //出现位置
	int NaluHeardLength;//Nalu标志出现的位置
};

#define DEPACKET_DATA_BUFFER_MAX_SIZE (4 * 1024 * 1024)

class rtp_depacket
{
public:
	bool                CheckVideoIsIFrame(int nVideoFrameType, unsigned char* szPVideoData, int nPVideoLength);
	int32_t             SplitterRtpDepacketBuffer(uint8_t* data, uint32_t datasize);
	NaluHeardPosition   nPosArray[MaxNaluBlockCount];
	unsigned int        nFindCount;
	int                 nPos1, nPos2, nNaluLength;
	int                 sclen, payloadlen;
	bool                bSetOptFlag;
	int                 nMediaTypeCodec;//媒体类型编码
	bool                bIsIFrameFlag;

	rtp_depacket(uint32_t ssrc, rtp_depacket_callback cb, void* userdata, uint32_t h);
    ~rtp_depacket();
	
	uint32_t get_ssrc() const { return m_ssrc;  }

	uint64_t get_lostpakcet() const { return m_lostpkt; }

	void set_payload(uint8_t payload, uint32_t streamtype);

	uint32_t get_payload(uint8_t payload);

	void set_mediaoption(const std::string& opt, const std::string& param);

	template<typename T> bool get_mediaoption(const std::string& opt, T& val);

	int32_t handle(uint8_t* data, uint32_t datasize);

private:
	bool is_seq_flip(uint16_t except, uint16_t actual);

	void handle_h264(uint8_t* data, uint32_t datasize);
	void h264_nalunit(uint8_t* data, uint32_t datasize);
	void h264_stapa(uint8_t* data, uint32_t datasize);
	void h264_fua(uint8_t* data, uint32_t datasize);

	void handle_h265(uint8_t* data, uint32_t datasize);
	void h265_nalunit(uint8_t* data, uint32_t datasize);
	void h265_ap(uint8_t* data, uint32_t datasize);
	void h265_fu(uint8_t* data, uint32_t datasize);
	void h265_paci(uint8_t* data, uint32_t datasize);
	void h265_nallike(uint8_t* data, uint32_t datasize);

	void handle_common(uint8_t* data, uint32_t datasize);
	void handle_aac(uint8_t* data, uint32_t datasize);
private:
	const uint32_t m_handle;
	const uint32_t m_ssrc;
	const rtp_depacket_callback m_cb;
	const void* m_userdata;

	std::unordered_map<uint8_t, uint32_t> m_payloadMap;
	std::unordered_map<std::string, std::string> m_mediaoptMap;

	_rtp_depacket_cb m_out;
	uint8_t m_buff[DEPACKET_DATA_BUFFER_MAX_SIZE];
	uint32_t m_bufsize;

	bool m_firstpkt;
	bool m_inlost;
	uint16_t m_nextseq;
	uint32_t m_lasttimestamp;

	uint64_t m_lostpkt;
};
typedef std::shared_ptr<rtp_depacket> rtp_depacket_ptr;

#endif