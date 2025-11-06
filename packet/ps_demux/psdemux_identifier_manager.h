#pragma once
#include <unordered_set>
#include <mutex>
#include <cstdint>
class PsDemuxIdentifierManager {
public:
    static PsDemuxIdentifierManager& getInstance();

    uint32_t Generate();
    void Recycle(uint32_t id);

private:
    PsDemuxIdentifierManager() = default;
    ~PsDemuxIdentifierManager() = default;
    PsDemuxIdentifierManager(const PsDemuxIdentifierManager&) = delete;
    PsDemuxIdentifierManager& operator=(const PsDemuxIdentifierManager&) = delete;

	std::unordered_set<uint32_t> m_identifier_set;
	std::mutex m_mutex;
	uint32_t m_next_id = 1;
};