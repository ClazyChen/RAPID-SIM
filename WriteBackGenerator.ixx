export module rapid.WriteBackGenerator;

import rapid.Packet;
import std;

export class WriteBackGenerator {
    std::vector<std::pair<std::byte, double>> m_write_back_probabilities;
    
    std::default_random_engine m_engine { std::random_device {}() };
    std::uniform_real_distribution<> m_distribution { 0, 1 };

public:
    WriteBackGenerator() = default;

    void initialize(std::initializer_list<std::pair<int, double>> l) {
        m_write_back_probabilities.clear();
        for (auto [id, prob] : l) {
            m_write_back_probabilities.push_back({static_cast<std::byte>(1 << id), prob});
        }
    }

    Packet set_write_back(Packet&& pkt) {
        for (auto& [mask, prob] : m_write_back_probabilities) {
            if (m_distribution(m_engine) < prob) {
                pkt.m_write_back_bitmap |= mask;
            }
        }
        return pkt;
    }
};