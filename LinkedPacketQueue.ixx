module;
#include <array>

export module rapid.LinkedPacketQueue;

import rapid.Packet;

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
    StaticQueue m_p2p, m_r2p;
    int m_back { 0 };

    void enqueue_static(StaticQueue& s_queue, Packet&& pkt) {
        if (s_queue.m_empty) {
            s_queue.m_empty = false;
            s_queue.m_front = s_queue.m_back = m_back;
        }
        else {
            m_next_link[s_queue.m_back] = m_back;
            s_queue.m_back = m_back;
        }
        m_queue[m_back] = std::move(pkt);
        m_next_link[m_back] = -1;
        m_back = (m_back + 1) % N;
    }

public:
    void reset() {
        m_p2p = m_r2p = StaticQueue {};
        m_back = 0;
        std::fill(m_next_link.begin(), m_next_link.end(), -1);
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