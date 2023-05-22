export module rapid.PacketQueue;

import rapid.Packet;
import std;

export template <size_t N>
    requires(std::has_single_bit(N))
class PacketQueue {
    std::array<Packet, N> m_queue;
    int m_front { 0 };
    int m_back { 0 };
    bool m_empty { true };
    const Packet m_empty_packet {};
    size_t m_size{ 0 };

public:
    PacketQueue() = default;
    Packet dequeue()
    {
        if (m_empty) {
            return Packet {};
        } else {
            auto pkt { std::move(m_queue[m_front]) };
            m_front = (m_front + 1) % N;
            m_empty = m_front == m_back;
            m_size--;
            return pkt;
        }
    }

    const Packet& front() const
    {
        if (m_empty) {
            return m_empty_packet;
        } else {
            return m_queue[m_front];
        }
    }

    bool enqueue(Packet&& pkt)
    {
        if (m_empty) {
            m_empty = false;
        } else {
            if (m_front == m_back) {
                return false;
            }
        }
        m_queue[m_back] = std::move(pkt);
        m_back = (m_back + 1) % N;
        m_size++;
        return true;
    }

    bool is_full() const
    {
        return !m_empty && m_front == m_back;
    }

    bool is_empty() const
    {
        return m_empty;
    }

    size_t size() const 
    {
        return m_size;
    }
};