module;
#include <bitset>

export module rapid.Piw;

import rapid.Packet;
import rapid.BlockQueue;

export template <std::byte PEER_MASK, size_t RING_LENGTH, size_t K = 2>
class Piw {
    constexpr const static std::byte m_peer_mask { PEER_MASK };
    constexpr const static short m_ring_length { RING_LENGTH };

    std::bitset<K> m_dirty_cam;

    BlockQueue<short, m_ring_length> m_cancel_dirty;
    BlockQueue<Packet, m_ring_length - 1> m_backward_packet;
    BlockQueue<Packet, m_ring_length - 1> m_write_back_packet;

public:
    Piw() = default;

    void cancel_dirty_step(short key)
    {
        m_cancel_dirty.enqueue(key);
        m_dirty_cam.reset(m_cancel_dirty.next());
    }

    std::tuple<Packet, Packet, Packet> next(Packet&& pkt)
    {
        Packet bp { m_backward_packet.next() };
        Packet wb { m_write_back_packet.next() };
        if (m_dirty_cam.test(pkt.m_key)) {
            pkt.set_backward_tag(m_peer_mask);
            m_backward_packet.enqueue(std::move(pkt));
            return { Packet {}, bp, wb };
        } else {
            Packet pipe { pkt };
            if (pkt.is_write_back_packet(m_peer_mask)) {
                m_write_back_packet.enqueue(std::move(pkt));
            }
            return { pipe, bp, wb };
        }
    }
};