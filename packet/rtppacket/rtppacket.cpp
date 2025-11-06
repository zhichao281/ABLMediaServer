/*
功能：
     rtp打包库接口函数实现，针对打包对象packet进行增加、获取、删除等等功能
日期    2025-09-03
作者    罗家兄弟
QQ      79941308
E-Mail  79941308@qq.com
*/

#include "stdafx.h"
#include "rtp_packet.h"
#include "packet.h"
#include "RtpPacketManager.h"
#include <mutex>
#include <memory>


RTP_PACKET_API int32_t rtp_packet_start(rtp_packet_callback cb, void* userdata, uint32_t* h)
{
    return RtpPacketManager::getInstance().Start(cb, userdata, h);
}

RTP_PACKET_API int32_t rtp_packet_stop(uint32_t h)
{
    return RtpPacketManager::getInstance().Stop(h);
}

RTP_PACKET_API int32_t rtp_packet_input(_rtp_packet_input* input)
{
    return RtpPacketManager::getInstance().Input(input);
}

RTP_PACKET_API int32_t rtp_packet_setsessionopt(_rtp_packet_sessionopt* opt)
{
    return RtpPacketManager::getInstance().SetSessionOpt(opt);
}

RTP_PACKET_API int32_t rtp_packet_resetsessionopt(_rtp_packet_sessionopt* opt)
{
    return RtpPacketManager::getInstance().ResetSessionOpt(opt);
}