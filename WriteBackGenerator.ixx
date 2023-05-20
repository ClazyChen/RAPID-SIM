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


export template <size_t K = 2, size_t WB_GAP = 1>
class FixedWriteBackGenerator {
    std::vector<std::pair<std::byte, double>> m_write_back_probabilities;
    std::array<size_t, K> flow_pkt_cnt; //��¼�����Ѿ����յİ��ĸ���
    size_t wb_gap{ WB_GAP }; //����Ѿ��������������wb_gap�������򽫸ð�����Ϊд�ذ�
public:
    FixedWriteBackGenerator() = default;

    void initialize(std::initializer_list<std::pair<int, double>> l) {
        m_write_back_probabilities.clear();
    }

    Packet set_write_back(Packet&& pkt) {
        auto flow_id = pkt.m_key;
        if (flow_id != 0) {
            flow_pkt_cnt[flow_id]++;
            if (flow_pkt_cnt[flow_id] > wb_gap) {
                flow_pkt_cnt[flow_id] = 0;
                pkt.m_write_back_bitmap |= std::byte(1);
            }
        }
        return pkt;
    }

};