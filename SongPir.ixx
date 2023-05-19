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
        bool m_block{ false }; //如果不是block但存在，则为schedule
        int head; //该流的包在schedule buffer中的头
        int tail; //该流的包在schedule buffer中的尾

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

    //std::array<PacketQueue<N>, K> m_schedule_buffer; // 每流的调度队列（多队列）
    Packet m_temp_pkt; // 回环过来的包

public:
    SongPir() {
        m_incoming_buffer_head = -1;
        m_incoming_buffer_tail = -1;
    };
    // 从流水线上来的包，进到frontbuffer
    //if (!pkt.is_empty()) {
    //    m_front_buffer.enqueue(std::move(pkt));
    //}
    // 优先调度回环的包
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
    //给调度的包分配序列号
    //auto seq_pkt { m_seq_id_marker.next(std::move(next_pkt)) };
    void set_temp_pkt(Packet pkt) {
        m_temp_pkt = pkt;
    }


    Packet next(Packet&& pkt)
    {
        // std::cout << m_dirty_state[1].head << std::endl;
        // std::cout << m_incoming_buffer_head << std::endl;
        // 从流水线上来的包，进到incoming buffer
        // 后面再选择是否将流水线上的pkt送进buffer，先处理回环的包
        //if (!pkt.is_empty()) {
        //    m_incoming_buffer.enqueue(std::move(pkt));
        //}
        //优先调度回环的包
        //std::cout << "m_next" << std::endl;
        //for (int i = 0; i < N; i++) {
        //    std::cout << m_circle_buffer.m_next[i] << std::endl;
        //}
        std::cout << m_circle_buffer.size() << std::endl;
        Packet next_pkt{ std::move(m_temp_pkt) };
        if (!next_pkt.is_empty()) {
            if (next_pkt.is_write_state) { //如果当前包只是写回状态，则不调度该包，（调度流水线上的包或schedulebuffer）
                auto& state{ m_dirty_state[next_pkt.m_key] };
                if (!state.m_exist) { //如果没有在dTable中
                    //则看写状态的包的序列号是否是期待的序列号（即只有一个包在写状态）
                    //如果是，则继续run，no action，如果不是，则block
                    if (next_pkt.get_seq_id() != last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt))) {
                        state.m_exist = true;
                        state.m_block = true;
                        state.m_seq_id = last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt));//seq_id设置为该流的最后一个包的seq_id，并且期待这个包到达
                    }
                }
                else { // 如果在dTable中，看是处于block状态还是schedule状态
                    //不管是什么状态，都要继续block，并且更新期待的seq_id
                    state.m_exist = true;
                    state.m_block = true;
                    state.m_seq_id =  last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt));
                }
                //（调度流水线上的包或schedule buffer）中的包
                for (size_t i = 1; i < K; i++) {
                    //如果schedule buffer中有可调度的流的包，则为其分配序列号，并调度
                    if ((m_dirty_state[i].m_exist && !m_dirty_state[i].m_block) && (m_dirty_state[i].head != -1)) {
                        //找这个可调度流的队列中第一个包
                        
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
                            //如果来的包不是空包，则加入incoming buffer中
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
                     // 如果schedule buffer 中没有，则调度从流水线上来的包
                    if (!pkt.is_empty()) {
                        //如果来的包不是空包，则加入incoming buffer中
                        auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                        if (next_id != -1) {
                            if (m_incoming_buffer_head == -1) {
                                m_incoming_buffer_head = next_id;
                            }
                            m_incoming_buffer_tail = next_id;
                        }
                    }
                    //之后判断incoming_buffer中是否有包
                    if (m_incoming_buffer_head != -1) { //如果incoming_buffer中有包
                        
                        auto pkt_and_nxt_id = m_circle_buffer.dequeue(m_incoming_buffer_head);
                        next_pkt = pkt_and_nxt_id.first;
                        auto nxt_id = pkt_and_nxt_id.second;
                        m_incoming_buffer_head = nxt_id;
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_tail = -1;
                        }
                        if (!next_pkt.is_empty()) {
                            auto& state{ m_dirty_state[next_pkt.m_key] };
                            if (state.m_exist) { //如果dTable中存在该流（一定是block），则加入调度队列
                                //由于从incomingbuffer中dequeue了一次，这里enqueue一定会成功，next_id 不会是-1
                                auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_dirty_state[next_pkt.m_key].tail);
                                m_dirty_state[next_pkt.m_key].tail = next_id;
                                return Packet{};
                            }
                            else { //如果不在dTable中，则正常调度
                                auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                                return std::move(seq_pkt);
                            }
                        }
                        else {
                            // empty包
                            auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                            return std::move(seq_pkt);
                        }
                    } 
                }
            }
            else {//如果是回环重传的包，则需要进行调度
                //
                if (!pkt.is_empty()) {
                    //如果流水线上来的包不是空包，则加入incoming buffer中
                    auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                    if (next_id != -1) {
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_head = next_id;
                        }
                        m_incoming_buffer_tail = next_id;
                    }
                }
                auto& state{ m_dirty_state[next_pkt.m_key] };
                if (state.m_block) { // 如果阻塞的包回环，则判断其是否是期待的序列号
                    if (next_pkt.get_seq_id() != last_seq_id(m_seq_id_marker.get_nxt_seq_id(next_pkt))) {//若不是，则继续阻塞，并调度包
                        //m_schedule_buffer[next_pkt.m_key].enqueue(std::move(pkt));
                        return std::move(next_pkt);
                    }
                    else { //若是，则将状态变为schedule，并调度包
                        //m_schedule_buffer[next_pkt.m_key].enqueue(std::move(pkt));
                        state.m_block = false;
                        return std::move(next_pkt);
                    }
                }
                //else { //如果可调度，则不会从ring上下来包
                //}
            }
        }
        else { //如果ring上没下来包，从schedule buffer中取
            //（调度流水线上的包或schedule buffer）中的包
            for (size_t i = 1; i < K; i++) {
                //如果schedule buffer中有可调度的流的包，则为其分配序列号，并调度
                if ((m_dirty_state[i].m_exist && !m_dirty_state[i].m_block) && (m_dirty_state[i].head != -1)) {
                    //找这个可调度流的队列中第一个包
                    
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
                        //如果来的包不是空包，则加入incoming buffer中
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
                // 如果schedule buffer 中没有，则调度从流水线上来的包
                if (!pkt.is_empty()) {
                    //如果来的包不是空包，则加入incoming buffer中
                    auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_incoming_buffer_tail);
                    //std::cout << "m_valid " << next_id << " " << m_circle_buffer.m_valid[next_id] << std::endl;
                    if (next_id != -1) {
                        if (m_incoming_buffer_head == -1) {
                            m_incoming_buffer_head = next_id;
                        }
                        m_incoming_buffer_tail = next_id;
                    }
                }
                //之后判断incoming_buffer中是否有包
                if (m_incoming_buffer_head != -1) { //如果incoming_buffer中有包
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
                        if (state.m_exist) { //如果dTable中存在该流（一定是block），则加入调度队列
                            //由于从incomingbuffer中dequeue了一次，这里enqueue一定会成功，next_id 不会是-1
                            auto next_id = m_circle_buffer.enqueue(std::move(pkt), m_dirty_state[next_pkt.m_key].tail);
                            m_dirty_state[next_pkt.m_key].tail = next_id;
                            return Packet{};
                        }
                        else { //如果不在dTable中，则正常调度
                            auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                            return std::move(seq_pkt);
                        }
                    }
                    else {
                        // empty包
                        auto seq_pkt{ m_seq_id_marker.next(std::move(next_pkt)) };
                        return std::move(seq_pkt);
                    }
                }
            }
        }
        return Packet{};
    }
};