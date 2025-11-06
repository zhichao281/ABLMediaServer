#include "RtpPacketManager.h"

RtpPacketManager& RtpPacketManager::getInstance() {
    static RtpPacketManager instance;
    return instance;
}

RtpPacketManager::RtpPacketManager() {}
RtpPacketManager::~RtpPacketManager() {}

std::shared_ptr<packet> RtpPacketManager::getPtrByID(uint32_t nID) {
    std::unique_lock<std::mutex> lock(m_packetPtrMapLock);
    auto it = m_packetPtrMap.find(nID);
    if (it != m_packetPtrMap.end())
        return it->second;
    return nullptr;
}

int32_t RtpPacketManager::Start(rtp_packet_callback cb, void* userdata, uint32_t* h) {
    if (cb == nullptr || h == nullptr)
        return e_rtppkt_err_invalidparam;

    auto ptr = std::make_shared<packet>();
    if (ptr == nullptr)
        return e_rtppkt_err_mallocsessionerror;

    {
        std::unique_lock<std::mutex> lock(m_packetPtrMapLock);
        *h = m_nBaseID;
        m_packetPtrMap[m_nBaseID] = ptr;
        ptr->nID = m_nBaseID;
        ptr->userdata = userdata;
        ptr->m_cb = cb;
        m_nBaseID++;
    }
    return e_rtppkt_err_noerror;
}

int32_t RtpPacketManager::Stop(uint32_t h) {
    std::unique_lock<std::mutex> lock(m_packetPtrMapLock);
    auto it = m_packetPtrMap.find(h);
    if (it != m_packetPtrMap.end()) {
        m_packetPtrMap.erase(it);
        return e_rtppkt_err_noerror;
    }
    return e_rtppkt_err_invalidsessionhandle;
}

int32_t RtpPacketManager::Input(_rtp_packet_input* input) {
    if (input == nullptr)
        return e_rtppkt_err_invalidparam;
    auto ptr = getPtrByID(input->handle);
    if (ptr == nullptr)
        return e_rtppkt_err_invalidsessionhandle;
    return ptr->handle(input->data, input->datasize, input->timestamp);
}

int32_t RtpPacketManager::SetSessionOpt(_rtp_packet_sessionopt* opt) {
    if (opt == nullptr)
        return e_rtppkt_err_invalidparam;
    auto ptr = getPtrByID(opt->handle);
    if (ptr == nullptr)
        return e_rtppkt_err_invalidsessionhandle;
    ptr->set_option(opt);
    return e_rtppkt_err_noerror;
}

int32_t RtpPacketManager::ResetSessionOpt(_rtp_packet_sessionopt* opt) {
    if (opt == nullptr)
        return e_rtppkt_err_invalidparam;
    auto ptr = getPtrByID(opt->handle);
    if (ptr == nullptr)
        return e_rtppkt_err_invalidsessionhandle;
    return e_rtppkt_err_noerror;
}