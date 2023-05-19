module;

#include <iostream>
#include <array>

export module rapid.CircleLinkQueue;

import rapid.Packet;

export template <size_t N>
    requires(std::has_single_bit(N))
//环形链式队列，队列中的每一个元素包括（等待调度的数据包、该包所属的流的下一个包所在的index）
class CircleLinkQueue {
public:
    std::array<Packet, N> m_queue;
    // incoming buffer和schedule buffer以链表的形式存放在queue中
    std::array<bool, N> m_valid;
    std::array<int, N> m_next; // 下一个（incoming buffer或该包所属流的schedule buffer的下一个包）
    size_t m_pos{ 0 }; //当前指针的位置（用于从前往后找buffer中空余的节点）
    //int m_back{ 0 };
    bool m_empty{ true };
    int m_size{ 0 };
//public:
    CircleLinkQueue() {
        for (size_t i{ 0 }; i < N; i++) {
            m_valid[i] = false;
            m_next[i] = -1;
        }
    };
    std::pair<Packet, int> dequeue(int index) 
    {
        if (index < 0) {
            perror("index < 0");
        }
        if (!m_valid.at(index)) {
            //应该不会出现这个
            // std::cout << index << std::endl;
            perror("index not valid");
            return std::make_pair(Packet{}, -1);
        }
        else {
            auto pkt = Packet{ std::move(m_queue.at(index)) };
            m_valid.at(index) = false;
            m_size--;
            if (m_size == 0) {
                m_empty = true;
            }
            return std::make_pair(pkt, m_next.at(index));
        }
    }

    int enqueue(Packet&& pkt, int last_index)
    {
        if (m_size == N) { // 队列已满
            return -1;
        }
        if (m_empty) {
            m_empty = false;
        }
        //寻找下一个空余的位置（循环查找）
        while (m_valid[m_pos]) { m_pos = (m_pos + 1) % N; }

        //插入包
        m_queue[m_pos] = std::move(pkt);
        m_valid[m_pos] = true;
        m_next[m_pos] = -1;
        if (last_index >= 0) {
            m_next[last_index] = m_pos;
        }
        m_size++;
        //返回插入的包在队列中的位置
        return (int)m_pos;
    }


    int size() const {
        return m_size;
    }

    constexpr bool is_empty() const
    {
        return m_empty;
    }
   
};