export module rapid.Pir;

import rapid.Packet;
import rapid.PacketQueue;
import rapid.LinkedPacketQueue;
import rapid.BlockQueue;
import rapid.RoundRobinQueue;
import std;

export template <std::byte DEST_MASK, size_t N, size_t CLOCK_MAX, size_t K = 2>
class Pir {
    constexpr const static std::byte m_dest_mask { DEST_MASK };
    constexpr const static short m_clock_max { CLOCK_MAX };

    std::array<int, K> m_stateful_ram;
    std::array<int, K> m_front_buffer_size;
    std::bitset<K> m_dirty_cam;
    std::bitset<K> m_suspend_cam;
    std::bitset<K> m_schedule_cam;
    std::array<LinkedPacketQueue<N>, K> m_buffer;
    int m_buffer_size { 0 };

    RoundRobinQueue<K> m_schedule_queue;
    BlockQueue<unsigned short, m_clock_max> m_block_queue;

    unsigned short m_scheduling_key { 0 };

    void ready_to_schedule(unsigned short key)
    {
        if constexpr (OUTPUT) {
            std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
            std::cout << g_clock << " READY SCH : " << key << std::endl;
        }
        m_buffer.at(key).merge_queues();
        m_schedule_queue.enqueue(key);
        m_schedule_cam.set(key);
        m_scheduling_key = key;
    }

    void complete_schedule(unsigned short key)
    {
        if constexpr (OUTPUT) {
            std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
            std::cout << g_clock << " COMPLETE SCH : " << key << std::endl;
        }
        m_schedule_cam.reset(key);
    }

    unsigned short timeout()
    {
        unsigned short key { m_block_queue.next() };
        if (key != 0) {
            if (m_front_buffer_size.at(key) > 0) {
                m_suspend_cam.set(key);
            } else {
                if (m_buffer.at(key).is_empty()) {
                    m_dirty_cam.reset(key);
                } else {
                    ready_to_schedule(key);
                }
            }
            if constexpr (OUTPUT) {
                std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
                std::cout << g_clock << " TIMEOUT : " << key << " (" << m_front_buffer_size.at(key) << ")" << std::endl;
            }
        }
        return key;
    }

    Packet schedule()
    {
        auto key { m_schedule_queue.dequeue() };
        if (key == 0) {
            return Packet {};
        }
        auto pkt { m_buffer.at(key).dequeue() };
        --m_buffer_size;
        if constexpr (OUTPUT) {
            std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
            std::cout << "DEQUEUE(" << m_buffer_size << ") " << g_clock << " : " << pkt << std::endl;
        }
        if (m_buffer.at(key).is_empty()) {
            m_dirty_cam.reset(key);
            complete_schedule(key);
        } else {
            m_schedule_queue.enqueue(key);
        }
        return pkt;
    }

    Packet enqueue(Packet&& pkt)
    {
        auto is_backward { pkt.is_backward_packet(m_dest_mask) };
        if (m_buffer_size < N) {
            ++m_buffer_size;
            if constexpr (OUTPUT) {
                std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
                std::cout << "enqueue(" << m_buffer_size << ") " << g_clock << " : " << pkt << std::endl;
            }
            if (is_backward && !m_schedule_cam.test(pkt.m_key)) {
                m_buffer.at(pkt.m_key).enqueue_r2p(std::move(pkt));
            } else {
                m_buffer.at(pkt.m_key).enqueue_p2p(std::move(pkt));
            }
        } else {
            if constexpr (OUTPUT) {
                std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
                std::cout << "drop " << g_clock << " : " << pkt << std::endl;
            }
            ++m_drop_packet_count;
        }
        if (is_backward) {
            if (--m_front_buffer_size.at(pkt.m_key) == 0 && m_suspend_cam.test(pkt.m_key)) {
                m_suspend_cam.reset(pkt.m_key);
                ready_to_schedule(pkt.m_key);
            }
        }
        return Packet {};
    }

public:
    int m_drop_packet_count { 0 };

    Pir() = default;

    void reset() {
        for (auto& queue : m_buffer) {
            queue.reset();
        }
    }

    void write_cam(Packet&& pkt)
    {
        if (!pkt.is_empty()) {
            m_dirty_cam.set(pkt.m_key);
            m_stateful_ram[pkt.m_key] = pkt.m_id;
            m_block_queue.enqueue(std::move(pkt.m_key));
            if constexpr (OUTPUT) {
                std::cout << "[ " << static_cast<int>(DEST_MASK) << " ]";
                std::cout << g_clock << " WRITE " << pkt.m_key << " FROM " << pkt.m_id << std::endl;
            }
        }
    }

    void count_backward_packet(unsigned short key)
    {
        if (key != 0) {
            ++m_front_buffer_size.at(key);
        }
    }

    unsigned short get_scheduling_key()
    {
        unsigned short key { m_scheduling_key };
        m_scheduling_key = 0;
        return key;
    }

    std::pair<Packet, short> next(Packet&& pkt)
    {
        unsigned short key { timeout() };
        if (pkt.is_empty()) {
            return { schedule(), key };
        } else {
            if (m_dirty_cam.test(pkt.m_key)) {
                return { enqueue(std::move(pkt)), key };
            } else {
                return { pkt, key };
            }
        }
    }
};