module;
#include <iostream>
#include <array>

export module rapid.SongPir;
import rapid.PacketQueue;
import rapid.Packet;
import rapid.SeqIdMarker;
import rapid.CircleLinkQueue;


export template <size_t N, size_t K = 2>
class SongPir {

    struct DirtyState {
        bool m_exist{ false };
        bool m_block{ false }; //�������block�����ڣ���Ϊschedule
        int head; //�����İ���schedule buffer�е�ͷ
        int tail; //�����İ���schedule buffer�е�β

        std::byte m_seq_id {};
        DirtyState() {
            head = -1;
            tail = -1;
        };
    };
    
    int m_incoming_buffer_head;
    int m_incoming_buffer_tail;

    std::array<DirtyState, K> m_dirty_state; // dTable

    //PacketQueue<N> m_incoming_buffer; 
    CircleLinkQueue<N * 32> m_circle_buffer;
    
    SeqIdMarker<K> m_seq_id_marker;

    //std::array<PacketQueue<N>, K> m_schedule_buffer; // ÿ���ĵ��ȶ��У�����У�
    Packet m_temp_pkt; // �ػ������İ�

public:
    SongPir() {
        m_incoming_buffer_head = -1;
        m_incoming_buffer_tail = -1;
    };
    // ����ˮ�������İ�������frontbuffer
    //if (!pkt.is_empty()) {
    //    m_front_buffer.enqueue(std::move(pkt));
    //}
    // ���ȵ��Ȼػ��İ�
    //Packet next_pkt { std::move(m_temp_pkt) };
    //if (next_pkt.is_empty()) {
    //    next_pkt = m_front_buffer.dequeue();
    //}
    //if (!next_pkt.is_empty()) {
    //    if constexpr (OUTPUT) {
    //        std::cout << "    " << g_clock << " "
    //                  << "next_pkt is " << next_pkt << std::endl;
    //    }
    //}
    //�����ȵİ��������к�
    //auto seq_pkt { m_seq_id_marker.next(std::move(next_pkt)) };
    void set_temp_pkt(Packet pkt) {
        m_temp_pkt = pkt;
    }


    Packet next(Packet&& pkt)
    {
        // std::cout << m_dirty_state[1].head << std::endl;
        // std::cout << m_incoming_buffer_head << std::endl;
        // ����ˮ�������İ�������incoming buffer
        // ������ѡ���Ƿ���ˮ���ϵ�pkt�ͽ�buffer���ȴ���ػ��İ�
        //if (!pkt.is_empty()) {
        //    m_incoming_buffer.enqueue(std::move(pkt));
        //}
        //���ȵ��Ȼػ��İ�
        //std::cout << "m_next" << std::endl;
        //for (int i = 0; i < N; i++) {
        //    std::cout << m_circle_buffer.m_next[i] << std::endl;
        //}
        std::cout << m_circle_buffer.size() << std::endl;
        Packet next_pkt{ std::move(m_temp_pkt) };
        if (!next_pkt.is_empty()) {
            if (next_pkt.is_write_state) { //�����ǰ��ֻ��д��״̬���򲻵��ȸð�����������ˮ���ϵİ���schedulebuffer��
                auto& state{ m_dirty_state[next_pkt.m_key] };
                if (!state.m_exist) { //���û����dTable��
                    //��д״̬�İ������к��Ƿ����ڴ������кţ���ֻ��һ������д״̬��
                    //����ǣ������run��no action��������ǣ���block
                    if (next_pkt.get_seq_id() != last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt))) {
                        state.m_exist = true;
                        state.m_block = true;
                        state.m_seq_id = last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt));//seq_id����Ϊ���������һ������seq_id�������ڴ����������
                    }
                }
                else { // �����dTable�У����Ǵ���block״̬����schedule״̬
                    //������ʲô״̬����Ҫ����block�����Ҹ����ڴ���seq_id
                    state.m_exist = true;
                    state.m_block = true;
                    state.m_seq_id =  last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt));
                }
                //��������ˮ���ϵİ���schedule buffer���еİ�
                for (size_t i = 1; i < K; i++) {
                    //���schedule buffer���пɵ��ȵ����İ�����Ϊ��������кţ�������
                    if ((m_dirty_state[i].m_exist && !m_dirty_state[i].m_block) && (m_dirty_state[i].head != -1)) {
                        //������ɵ������Ķ����е�һ����
                        
                        auto pkt_and_nxt_id = m_circle_buffer.dequeue(m_dirty_state[i].head);
                        next_pkt = pkt_and_nxt_id.first;
                        auto nxt_id = pkt_and_nxt_id.second;
                        m_dirty_state[i].head = nxt_id;
                        if (m_dirty_state[i].head == -1) {
                            m_dirty_state[i].m_exist = false;
                            m_dirty_state[i].tail = -1;
                        }
                        auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };

                        if (!pkt.is_empty()) {
                            //������İ����ǿհ��������incoming buffer��
                            auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                            if (next_id != -1) {
                                if (m_incoming_buffer_head == -1) {
                                    m_incoming_buffer_head = next_id;
                                }
                                m_incoming_buffer_tail = next_id;
                            }
                        }

                        return std::move(seq_pkt);
                    }
                }
                if (next_pkt.is_empty()) { 
                     // ���schedule buffer ��û�У�����ȴ���ˮ�������İ�
                    if (!pkt.is_empty()) {
                        //������İ����ǿհ��������incoming buffer��
                        auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                        if (next_id != -1) {
                            if (m_incoming_buffer_head == -1) {
                                m_incoming_buffer_head = next_id;
                            }
                            m_incoming_buffer_tail = next_id;
                        }
                    }
                    //֮���ж�incoming_buffer���Ƿ��а�
                    if (m_incoming_buffer_head != -1) { //���incoming_buffer���а�
                        
                        auto pkt_and_nxt_id = m_circle_buffer.dequeue(m_incoming_buffer_head);
                        next_pkt = pkt_and_nxt_id.first;
                        auto nxt_id = pkt_and_nxt_id.second;
                        m_incoming_buffer_head = nxt_id;
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_tail = -1;
                        }
                        if (!next_pkt.is_empty()) {
                            auto& state{ m_dirty_state[next_pkt.m_key] };
                            if (state.m_exist) { //���dTable�д��ڸ�����һ����block�����������ȶ���
                                //���ڴ�incomingbuffer��dequeue��һ�Σ�����enqueueһ����ɹ���next_id ������-1
                                auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_dirty_state[next_pkt.m_key].tail);
                                m_dirty_state[next_pkt.m_key].tail = next_id;
                                return Packet{};
                            }
                            else { //�������dTable�У�����������
                                auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                                return std::move(seq_pkt);
                            }
                        }
                        else {
                            // empty��
                            auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                            return std::move(seq_pkt);
                        }
                    } 
                }
            }
            else {//����ǻػ��ش��İ�������Ҫ���е���
                //
                if (!pkt.is_empty()) {
                    //�����ˮ�������İ����ǿհ��������incoming buffer��
                    auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                    if (next_id != -1) {
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_head = next_id;
                        }
                        m_incoming_buffer_tail = next_id;
                    }
                }
                auto& state{ m_dirty_state[next_pkt.m_key] };
                if (state.m_block) { // ��������İ��ػ������ж����Ƿ����ڴ������к�
                    if (next_pkt.get_seq_id() != last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt))) {//�����ǣ�����������������Ȱ�
                        //m_schedule_buffer[next_pkt.m_key].enqueue(std::move(pkt));
                        return std::move(next_pkt);
                    }
                    else { //���ǣ���״̬��Ϊschedule�������Ȱ�
                        //m_schedule_buffer[next_pkt.m_key].enqueue(std::move(pkt));
                        state.m_block = false;
                        return std::move(next_pkt);
                    }
                }
                //else { //����ɵ��ȣ��򲻻��ring��������
                //}
            }
        }
        else { //���ring��û����������schedule buffer��ȡ
            //��������ˮ���ϵİ���schedule buffer���еİ�
            for (size_t i = 1; i < K; i++) {
                //���schedule buffer���пɵ��ȵ����İ�����Ϊ��������кţ�������
                if ((m_dirty_state[i].m_exist && !m_dirty_state[i].m_block) && (m_dirty_state[i].head != -1)) {
                    //������ɵ������Ķ����е�һ����
                    
                    auto pkt_and_nxt_id = m_circle_buffer.dequeue(m_dirty_state[i].head);
                    next_pkt = pkt_and_nxt_id.first;
                    auto nxt_id = pkt_and_nxt_id.second;
                    m_dirty_state[i].head = nxt_id;
                    if (m_dirty_state[i].head == -1) {
                        m_dirty_state[i].m_exist = false;
                        m_dirty_state[i].tail = -1;
                    }
                    auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };

                    if (!pkt.is_empty()) {
                        //������İ����ǿհ��������incoming buffer��
                        auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                        if (next_id != -1) {
                            if (m_incoming_buffer_head == -1) {
                                m_incoming_buffer_head = next_id;
                            }
                            m_incoming_buffer_tail = next_id;
                        }
                    }
                    return std::move(seq_pkt);
                }
            }
            if (next_pkt.is_empty()) {
                // ���schedule buffer ��û�У�����ȴ���ˮ�������İ�
                if (!pkt.is_empty()) {
                    //������İ����ǿհ��������incoming buffer��
                    auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                    //std::cout << "m_valid " << next_id << " " << m_circle_buffer.m_valid[next_id] << std::endl;
                    if (next_id != -1) {
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_head = next_id;
                        }
                        m_incoming_buffer_tail = next_id;
                    }
                }
                //֮���ж�incoming_buffer���Ƿ��а�
                if (m_incoming_buffer_head != -1) { //���incoming_buffer���а�
                    //std::cout << m_incoming_buffer_head << std::endl;
                    //std::cout << "incoming buffer head " << m_circle_buffer.m_valid[m_incoming_buffer_head] << std::endl;
                    auto pkt_and_nxt_id = m_circle_buffer.dequeue(m_incoming_buffer_head);
                    next_pkt = pkt_and_nxt_id.first;
                    auto nxt_id = pkt_and_nxt_id.second;
                    m_incoming_buffer_head = nxt_id;
                    if (m_incoming_buffer_head == -1) {
                        m_incoming_buffer_tail = -1;
                    }
                    //std::cout << "nxt id" << nxt_id << std::endl;
                    if (!next_pkt.is_empty()) {
                        auto& state{ m_dirty_state[next_pkt.m_key] };
                        if (state.m_exist) { //���dTable�д��ڸ�����һ����block�����������ȶ���
                            //���ڴ�incomingbuffer��dequeue��һ�Σ�����enqueueһ����ɹ���next_id ������-1
                            auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_dirty_state[next_pkt.m_key].tail);
                            m_dirty_state[next_pkt.m_key].tail = next_id;
                            return Packet{};
                        }
                        else { //�������dTable�У�����������
                            auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                            return std::move(seq_pkt);
                        }
                    }
                    else {
                        // empty��
                        auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                        return std::move(seq_pkt);
                    }
                }
            }
        }
        return Packet{};
    }
};