/*
功能：
	rtp解包库接口函数实现，针对打包对象depacket进行增加、获取、删除等等功能
*/
#include "stdafx.h"
#include "rtp_depacket.h"
#include "RtpDePacketManager.h"

RTP_DEPACKET_API int32_t rtp_depacket_start(rtp_depacket_callback cb, void* userdata, uint32_t* h) {
    return RtpDePacketManager::getInstance().Start(cb, userdata, h);
}

RTP_DEPACKET_API int32_t rtp_depacket_stop(uint32_t h) {
    return RtpDePacketManager::getInstance().Stop(h);
}

RTP_DEPACKET_API int32_t rtp_depacket_input(uint32_t h, uint8_t* data, uint32_t datasize) {
    return RtpDePacketManager::getInstance().Input(h, data, datasize);
}

RTP_DEPACKET_API int32_t rtp_depacket_setpayload(uint32_t h, uint8_t payload, uint32_t streamtype) {
    return RtpDePacketManager::getInstance().SetPayload(h, payload, streamtype);
}

RTP_DEPACKET_API int32_t rtp_depacket_setmediaoption(uint32_t h, int8_t* opt, int8_t* parm) {
    return RtpDePacketManager::getInstance().SetMediaOption(h, opt, parm);
}