module;
#include <array>
#include <iostream>

export module rapid.RoundRobinQueue;

export template <size_t K>
class RoundRobinQueue {
    std::array<unsigned short, K> m_schedule_queue;
    int m_front { 0 };
    int m_back { 0 };

public:
    RoundRobinQueue() = default;

    bool is_empty()
    {
        return m_front == m_back;
    }

    void enqueue(unsigned short key)
    {
        m_schedule_queue[m_back] = key;
        if (++m_back == K) {
            m_back = 0;
        }
    }

    unsigned short dequeue()
    {
        if (is_empty()) {
            return 0;
        }
        unsigned short key { m_schedule_queue[m_front] };
        if (++m_front == K) {
            m_front = 0;
        }
        return key;
    }
};