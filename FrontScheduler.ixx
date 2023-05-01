module;
#include <utility>
#include <iostream>

export module rapid.FrontScheduler;

import rapid.Packet;
import rapid.PacketQueue;

export template <size_t N>
class FrontScheduler {
    PacketQueue<N> m_queue;

public:
    int m_drop_packet_count { 0 };

    FrontScheduler() = default;

    std::pair<Packet, short> next(Packet&& pipeline_packet, Packet&& backward_packet)
    {
        short backward_key { backward_packet.m_key };
        if (!backward_packet.is_empty()) {
            if (!m_queue.enqueue(std::move(backward_packet))) {
                ++m_drop_packet_count;
                backward_key = 0;
                std::cout << "back-drop " << backward_packet << std::endl;
            } else {
                //std::cout << "backward: " << backward_packet << std::endl;
            }
        }
        if (pipeline_packet.is_empty()) {
            return { m_queue.dequeue(), backward_key };
        } else {
            return { pipeline_packet, backward_key };
        }
    }
};