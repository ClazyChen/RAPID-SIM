module;

#include <iostream>
#include <array>

export module rapid.CircleLinkQueue;

import rapid.Packet;

export template <size_t N>
    requires(std::has_single_bit(N))
//������ʽ���У������е�ÿһ��Ԫ�ذ������ȴ����ȵ����ݰ����ð�������������һ�������ڵ�index��
class CircleLinkQueue {
public:
    std::array<Packet, N> m_queue;
    // incoming buffer��schedule buffer���������ʽ�����queue��
    std::array<bool, N> m_valid;
    std::array<int, N> m_next; // ��һ����incoming buffer��ð���������schedule buffer����һ������
    size_t m_pos{ 0 }; //��ǰָ���λ�ã����ڴ�ǰ������buffer�п���Ľڵ㣩
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
            //Ӧ�ò���������
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
        if (m_size == N) { // ��������
            return -1;
        }
        if (m_empty) {
            m_empty = false;
        }
        //Ѱ����һ�������λ�ã�ѭ�����ң�
        while (m_valid[m_pos]) { m_pos = (m_pos + 1) % N; }

        //�����
        m_queue[m_pos] = std::move(pkt);
        m_valid[m_pos] = true;
        m_next[m_pos] = -1;
        if (last_index >= 0) {
            m_next[last_index] = m_pos;
        }
        m_size++;
        //���ز���İ��ڶ����е�λ��
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