export module rapid.LinkedPacketQueue;

import rapid.Packet;
import std;

export template <size_t N>
    requires(std::has_single_bit(N))
class LinkedPacketQueue {
    struct StaticQueue {
        int m_front { 0 };
        int m_back { 0 };
        bool m_empty { true };
    };

    std::array<Packet, N> m_queue;
    std::array<int, N> m_next_link;
    std::array<int, N> m_empty_positions;
    StaticQueue m_p2p, m_r2p;
    int m_empty_positions_front { 0 };
    int m_empty_positions_back { 0 };

    void enqueue_static(StaticQueue& s_queue, Packet&& pkt) {
        auto pos { m_empty_positions[m_empty_positions_front] };
        if (s_queue.m_empty) {
            s_queue.m_empty = false;
            s_queue.m_front = s_queue.m_back = pos;
        }
        else {
            m_next_link[s_queue.m_back] = pos;
            s_queue.m_back = pos;
        }
        m_queue[pos] = std::move(pkt);
        m_next_link[pos] = -1;
        m_empty_positions_front = (m_empty_positions_front + 1) % N;
    }

public:
    void reset() {
        m_p2p = m_r2p = StaticQueue {};
        m_empty_positions_front = m_empty_positions_back = 0;
        std::fill(m_next_link.begin(), m_next_link.end(), -1);
        for (int i{ 0 }; i < N; ++i) {
            m_empty_positions[i] = i;
        }
    }

    LinkedPacketQueue()
    {
        reset();
    }

    void enqueue_p2p(Packet&& pkt) {
        enqueue_static(m_p2p, std::move(pkt));
    }

    void enqueue_r2p(Packet&& pkt) {
        enqueue_static(m_r2p, std::move(pkt));
    }

    void merge_queues() {
        if (!m_r2p.m_empty) {
            if (!m_p2p.m_empty) {
                m_next_link[m_r2p.m_back] = m_p2p.m_front;
                m_p2p.m_front = m_r2p.m_front;
            } else {
                m_p2p = m_r2p;
            }
        }
        m_r2p.m_empty = true;
    }

    Packet dequeue() {
        if (m_p2p.m_empty) {
            return Packet {};
        } else {
            auto pkt { std::move(m_queue[m_p2p.m_front]) };
            m_empty_positions[m_empty_positions_back] = m_p2p.m_front;
            m_empty_positions_back = (m_empty_positions_back + 1) % N;
            m_p2p.m_front = m_next_link[m_p2p.m_front];
            if (m_p2p.m_front == -1) {
                m_p2p.m_empty = true;
            }
            return pkt;
        }
    }

    bool is_empty() const {
        return m_p2p.m_empty && m_r2p.m_empty;
    }

};