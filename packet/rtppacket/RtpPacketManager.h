#pragma once
#include "packet.h"
#include "rtp_packet.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class RtpPacketManager {
public:
    static RtpPacketManager& getInstance();

    int32_t Start(rtp_packet_callback cb, void* userdata, uint32_t* h);
    int32_t Stop(uint32_t h);
    int32_t Input(_rtp_packet_input* input);
    int32_t SetSessionOpt(_rtp_packet_sessionopt* opt);
    int32_t ResetSessionOpt(_rtp_packet_sessionopt* opt);

private:
    RtpPacketManager();
    ~RtpPacketManager();

    std::shared_ptr<packet> getPtrByID(uint32_t nID);

    std::unordered_map<uint32_t, std::shared_ptr<packet>> m_packetPtrMap;
    std::mutex m_packetPtrMapLock;
    uint32_t m_nBaseID = 1;
};