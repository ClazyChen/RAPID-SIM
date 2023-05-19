export module rapid.BlockQueue;

import rapid.Packet;
import std;

export template <typename T, size_t Len>
    requires(std::is_integral_v<T> || std::is_same_v<T, Packet>)
class BlockQueue {
    constexpr const static short m_length { Len };
    constexpr const static short m_timers_size { Len + 1 };
    unsigned short m_clock_max { 0 };
    unsigned short m_clock { 0 };
    std::array<T, m_timers_size> m_timers;

public:
    BlockQueue()
        : m_clock_max(m_length)
    {
        if constexpr (std::is_integral_v<T>) {
            std::fill(m_timers.begin(), m_timers.end(), 0);
        }
        else {
            std::fill(m_timers.begin(), m_timers.end(), Packet {});
        }
    }

    void enqueue(T&& key)
    {
        m_timers.at((m_clock + m_clock_max) % m_timers_size) = std::move(key);
    }

    T next()
    {
        m_clock = (m_clock + 1) % m_timers_size;
        if constexpr (std::is_integral_v<T>) {
            if (m_timers.at(m_clock) != 0) {
                T key { m_timers.at(m_clock) };
                m_timers.at(m_clock) = 0;
                return key;
            } else {
                return 0;
            }
        } else {
            if (!m_timers.at(m_clock).is_empty()) {
                T key { m_timers.at(m_clock) };
                m_timers.at(m_clock) = T {};
                return key;
            } else {
                return T {};
            }
        }
    }
};