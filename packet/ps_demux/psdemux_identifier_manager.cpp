#include "psdemux_identifier_manager.h"

PsDemuxIdentifierManager& PsDemuxIdentifierManager::getInstance() {
	static PsDemuxIdentifierManager instance;
	return instance;
}

uint32_t PsDemuxIdentifierManager::Generate() {
	std::lock_guard<std::mutex> lg(m_mutex);
	for (;;) {
		auto it = m_identifier_set.find(m_next_id);
		if ((m_identifier_set.end() == it) && (0 != m_next_id)) {
			auto ret = m_identifier_set.insert(m_next_id);
			if (ret.second) {
				break;
			}
		}
		else {
			++m_next_id;
		}
	}
	return m_next_id++;
}

void PsDemuxIdentifierManager::Recycle(uint32_t id) {
	std::lock_guard<std::mutex> lg(m_mutex);
	auto it = m_identifier_set.find(id);
	if (m_identifier_set.end() != it) {
		m_identifier_set.erase(it);
	}
}