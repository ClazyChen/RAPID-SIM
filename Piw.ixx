export module rapid.Piw;

import rapid.Packet;
import rapid.BlockQueue;
import std;

export template <std::byte PEER_MASK, size_t RING_LENGTH, size_t K = 2, bool ENABLE_UNWRITEABLE = false>
class Piw {
    constexpr const static std::byte m_peer_mask { PEER_MASK };
    constexpr const static unsigned short m_ring_length { RING_LENGTH };

    std::bitset<K> m_dirty_cam;
    std::bitset<K> m_unwritable_cam;

    BlockQueue<unsigned short, m_ring_length> m_cancel_dirty;
    BlockQueue<Packet, m_ring_length - 1> m_backward_packet;
    BlockQueue<Packet, m_ring_length - 1> m_write_back_packet;

public:
    Piw() = default;

    void cancel_dirty_step(unsigned short key)
    {
        m_cancel_dirty.enqueue(std::move(key));
        m_dirty_cam.reset(m_cancel_dirty.next());
    }

    void reset_scheduling(unsigned short key) {
        m_unwritable_cam.reset(key);
    }

    std::tuple<Packet, Packet, Packet> next(Packet&& pkt)
    {
        Packet bp { m_backward_packet.next() };
        Packet wb { m_write_back_packet.next() };
        if (!pkt.is_empty() && m_dirty_cam.test(pkt.m_key)) {
            pkt.set_backward_tag(m_peer_mask);
            m_backward_packet.enqueue(std::move(pkt));
            return { Packet {}, bp, wb };
        } else {
            Packet pipe { pkt };
            if (pkt.is_write_back_packet(m_peer_mask)) {
                if (!m_unwritable_cam.test(pkt.m_key)) {
                    m_dirty_cam.set(pkt.m_key);
                    m_write_back_packet.enqueue(std::move(pkt));
                }
            } else {
                if constexpr (ENABLE_UNWRITEABLE) {
                    m_unwritable_cam.set(pkt.m_key);
                }
            }
            return { pipe, bp, wb };
        }
    }
};