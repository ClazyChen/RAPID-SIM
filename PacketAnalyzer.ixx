export module rapid.PacketAnalyzer;

import rapid.Packet;
import std;

export template <size_t K = 2>
class PacketAnalyzer {

    std::array<int, K> m_last_packet_id;
    int m_wrong_order_count { 0 };

public:
    PacketAnalyzer() = default;
    void receive_packet(Packet&& pkt)
    {
        if (!pkt.is_empty()) {
            if (m_last_packet_id.at(pkt.m_key) > pkt.m_id) {
                ++m_wrong_order_count;
                std::cout << "error! : " << pkt << " @ " << m_last_packet_id.at(pkt.m_key) << std::endl;
            } else {
                m_last_packet_id.at(pkt.m_key) = pkt.m_id;
            }
        }
    }

    int get_wrong_order_count() const
    {
        return m_wrong_order_count;
    }

    void reset()
    {
        std::fill(m_last_packet_id.begin(), m_last_packet_id.end(), 0);
        m_wrong_order_count = 0;
    }
};