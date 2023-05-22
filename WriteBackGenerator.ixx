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


export template <size_t K = 2>
class FixedWriteBackGenerator {
    //std::vector<std::pair<std::byte, double>> m_write_back_probabilities;
    std::array<size_t, K> m_flow_pkt_cnt; //记录该流已经接收的包的个数
     //如果已经接受了这个流的wb_gap个包，则将该包设置为写回包
public:
    size_t m_wb_gap{ 9 };
    FixedWriteBackGenerator() = default;

    void initialize(size_t wb_gap) {
        // m_write_back_probabilities.clear();
        m_wb_gap = wb_gap;
    }

    void initialize(std::initializer_list<std::pair<int, double>> l) {
        std::cout << "used wrong initialize" << std::endl;
        //m_write_back_probabilities.clear();
    }

    Packet set_write_back(Packet&& pkt) {
        // std::cout << m_wb_gap << std::endl;
        auto flow_id = pkt.m_key;
        if (flow_id != 0) {
            m_flow_pkt_cnt[flow_id]++;
            if (m_flow_pkt_cnt[flow_id] > m_wb_gap) {
                m_flow_pkt_cnt[flow_id] = 0;
                pkt.m_write_back_bitmap |= std::byte(1);
            }
        }
        return pkt;
    }

};