export module rapid.FrontScheduler;

import rapid.Packet;
import rapid.PacketQueue;
import std;

export template <size_t N>
class FrontScheduler {
    PacketQueue<N> m_queue;

public:
    int m_drop_packet_count { 0 };

    FrontScheduler() = default;

    std::pair<Packet, unsigned short> next(Packet&& pipeline_packet, Packet&& backward_packet)
    {
        unsigned short backward_key { backward_packet.m_key };
        if (!backward_packet.is_empty()) {
            if (!m_queue.enqueue(std::move(backward_packet))) {
                ++m_drop_packet_count;
                backward_key = 0;
                if constexpr (OUTPUT) {
                    std::cout << "back-drop " << g_clock << " : " << backward_packet << std::endl;
                }
            } else {
                if constexpr (OUTPUT) {
                    std::cout << "backward  " << g_clock << " : " << backward_packet << std::endl;
                }
            }
        }
        if (pipeline_packet.is_empty()) {
            auto pkt { m_queue.dequeue() };
            if (!pkt.is_empty()) {
                if constexpr (OUTPUT) {
                    std::cout << "FRONT-DEQUEUE " << g_clock << " : " << pkt << std::endl;
                }
            }
            return { pkt, backward_key };
        } else {
            return { pipeline_packet, backward_key };
        }
    }
};