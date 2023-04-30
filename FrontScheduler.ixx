module;
#include <utility>

export module rapid.FrontScheduler;

import rapid.Packet;
import rapid.PacketQueue;

export template <size_t N>
class FrontScheduler {
    PacketQueue<N> m_queue;

public:
    int m_drop_packet_count { 0 };

    FrontScheduler() = default;

    Packet next(Packet&& pipeline_packet, Packet&& backward_packet)
    {
        if (!backward_packet.is_empty()) {
            if (!m_queue.enqueue(std::move(backward_packet))) {
                ++m_drop_packet_count;
            }
        }
        if (pipeline_packet.is_empty()) {
            return m_queue.dequeue();
        } else {
            return pipeline_packet;
        }
    }
};