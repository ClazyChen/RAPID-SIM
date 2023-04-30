module;
#include <array>
#include <bitset>

export module rapid.Pir;

import rapid.Packet;
import rapid.PacketQueue;
import rapid.BlockQueue;

export template <std::byte DEST_MASK, size_t N, size_t CLOCK_MAX, size_t K = 2>
class Pir {
    constexpr const static std::byte m_dest_mask { DEST_MASK };
    constexpr const static short m_clock_max { CLOCK_MAX };

    std::array<int, K> m_stateful_ram;
    std::array<int, K> m_front_buffer_size;
    std::bitset<K> m_dirty_cam;
    std::bitset<K> m_suspend_cam;
    std::bitset<K> m_schedule_cam;
    std::array<PacketQueue<N>, K> m_buffer;
    int m_buffer_size { 0 };

    int m_round_robin { 0 };
    void round_robin_step()
    {
        if constexpr (K > 2) {
            for (int next { m_round_robin + 1 }; next != m_round_robin; ++next) {
                if (next == K) {
                    next = 0;
                }
                if (m_schedule_cam.test(next)) {
                    m_round_robin = next;
                    return;
                }
            }
        }
    }

    BlockQueue<short, m_clock_max> m_block_queue;

    short timeout()
    {
        short key { m_block_queue.next() };
        if (key != 0) {
            if (m_buffer.at(key).is_empty()) {
                m_dirty_cam.reset(key);
            } else {
                if (m_front_buffer_size.at(key) > 0) {
                    m_suspend_cam.set(key);
                } else {
                    m_schedule_cam.set(key);
                }
            }
        }
        return key;
    }

    Packet schedule()
    {
        auto pkt { m_buffer.at(m_round_robin).dequeue() };
        if (!pkt.is_empty()) {
            --m_buffer_size;
            if (m_buffer.at(m_round_robin).is_empty()) {
                m_dirty_cam.reset(m_round_robin);
                m_schedule_cam.reset(m_round_robin);
            }
            round_robin_step();
        }
        return pkt;
    }

    Packet enqueue(Packet&& pkt)
    {
        if (m_buffer_size < N) {
            ++m_buffer_size;
            m_buffer.at(pkt.m_key).enqueue(std::move(pkt));
        } else {
            ++m_drop_packet_count;
        }
        if (pkt.is_backward_packet(m_dest_mask)) {
            if (--m_front_buffer_size.at(pkt.m_key) == 0 && m_suspend_cam.test(pkt.m_key)) {
                m_suspend_cam.reset(pkt.m_key);
                m_schedule_cam.set(pkt.m_key);
            }
        }
        return Packet {};
    }

public:
    int m_drop_packet_count { 0 };

    Pir() = default;

    void write_cam(Packet&& pkt)
    {
        m_dirty_cam.set(pkt.m_key);
        m_stateful_ram[pkt.m_key] = pkt.m_id;
        m_block_queue.enqueue(pkt.m_key);
    }

    std::pair<Packet, short> next(Packet&& pkt)
    {
        short key { timeout() };
        if (pkt.is_empty()) {
            return { schedule(), key };
        } else {
            if (pkt.is_backward_packet(m_dest_mask)) {
                return { enqueue(std::move(pkt)), key };
            } else {
                return { pkt, key };
            }
        }
    }
};