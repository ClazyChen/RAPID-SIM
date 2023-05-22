export module rapid.RoundRobinQueue;

import std;

export template <size_t K>
class RoundRobinQueue {
    std::array<unsigned short, K> m_schedule_queue;
    int m_front { 0 };
    int m_back { 0 };
    size_t m_size{ 0 };

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
        m_size++;
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
        m_size--;
        return key;
    }

    bool remove( unsigned short key )
    {
        if (is_empty())
            return false;
        
        auto size = m_size;
        for (int i = 0; i < size; i++) {
            auto x = dequeue();
            if (x == key) {
                //std::cout << "successfully remove" << std::endl;
                return true;
            } 
            enqueue(x);
        }
        return false;
    }


    size_t size() const {
        return m_size;
    }
};