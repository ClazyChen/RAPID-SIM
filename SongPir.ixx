export module rapid.SongPir;

import rapid.Packet;
import rapid.PacketQueue;
import rapid.SeqIdMarker;
import rapid.VirtualPipeline;
import rapid.RoundRobinQueue;
import std;
#include <assert.h>
export template <size_t N, size_t K = 2>
class SongPir {
    PacketQueue<N> m_incoming_buffer;
    SeqIdMarker<K> m_seq_id_marker;
    std::bitset<K> m_dirty_cam;
    std::bitset<K> m_schedule_cam;
    std::array<PacketQueue<N>, K> m_dirty_buffer;
    int m_buffer_size{ 0 };
    RoundRobinQueue<K> m_schedule_queue;


    unsigned short find_nxt_schedule() {
        auto size = m_schedule_queue.size();
        for (int i = 0; i < size; i++) {
            auto x = m_schedule_queue.dequeue();
            //if (x != 0) {
            //    return x;
            //} 
            if (m_dirty_buffer.at(x).is_empty()) {
                m_dirty_cam.reset(x);
                m_schedule_cam.reset(x);
            }
            else {
                return x;
            }
        }
        return 0;
    }

    Packet schedule() {
        //auto key { m_schedule_queue.dequeue() };
        auto key{ find_nxt_schedule() };
        if (key == 0) {
            return Packet{};
        }
        dirty_buffer_snd++;
        auto pkt{ m_dirty_buffer.at(key).dequeue() };
        --m_buffer_size;

        schedule_fout << "cycle: " << cnt_cycle << " schedule flow: " << key << " pkt key: " << pkt.m_key;
        if (m_dirty_buffer.at(key).is_empty()) {
            m_dirty_cam.reset(key);
            m_schedule_cam.reset(key);
            schedule_fout << " empty" << std::endl;
            // std::cout << "empty" << std::endl;
        }
        else {
            m_schedule_queue.enqueue(key);
            schedule_fout << " not empty" << std::endl;
            //std::cout << "not empty" << std::endl;
        }
        
        return pkt;
    }

    void enqueue(Packet&& pkt) {
        if (m_buffer_size < N) {
            ++m_buffer_size;
            dirty_buffer_rcv++;
            // std::cout << "cycle: " << cnt_cycle << " enqueue dirty buffer " << pkt.m_key << std::endl;
            m_dirty_buffer.at(pkt.m_key).enqueue(std::move(pkt));
        }
        else {
            ++m_drop_packet_count;
            std::cout << "shouldn't be here" << std::endl;
        }
    }

    std::ofstream ib_fout;
    std::ofstream db_fout;
    size_t cnt_cycle{ 0 };
    std::ofstream schedule_fout;
    std::ofstream wb_fout;
    std::ofstream bp_fout;

    size_t incoming_buffer_rcv{ 0 };
    size_t incoming_buffer_snd{ 0 };
    size_t incoming_buffer_to_dirty_buffer{ 0 };
    size_t incoming_buffer_to_pipeline{ 0 };
    size_t dirty_buffer_rcv{ 0 };
    size_t dirty_buffer_snd{ 0 };

public:
    int m_drop_packet_count { 0 };
    //SongPir() = default;
    SongPir() {
        ib_fout.open("ib_size.txt", std::ios_base::out);
        db_fout.open("db_size.txt", std::ios_base::out);
        schedule_fout.open("schedule_pkt.txt", std::ios_base::out);
        wb_fout.open("wb_info.txt", std::ios_base::out);
        bp_fout.open("bp_info.txt", std::ios_base::out);
    }


    bool is_full() const {
        return m_incoming_buffer.is_full();
    }

    void write_cam(Packet&& pkt) {
        cnt_cycle++;
        int expect_id = (int)last_seq_id( m_seq_id_marker.get_seq_id(pkt.m_key));
        wb_fout << "cycle: " << cnt_cycle << " flow id: " << pkt.m_key  << " is_write_back: " << !pkt.is_empty() << " seq id: " << (int)pkt.get_seq_id() << " expect id: " << expect_id << std::endl;
        if (!pkt.is_empty()) {
            if (pkt.get_seq_id() != last_seq_id(m_seq_id_marker.get_seq_id(pkt.m_key))) {
                //std::cout << "cycle: " << cnt_cycle << " error" << std::endl;
                m_dirty_cam.set(pkt.m_key);
                m_schedule_cam.reset(pkt.m_key);
                m_schedule_queue.remove(pkt.m_key);
            }
        }
    }

    // resubmit > schedule > incoming
    Packet next(Packet&& incoming_pkt, Packet&& resubmit_pkt) {

        ib_fout << "cycle: " << cnt_cycle << " incoming_buffer_size: " << m_incoming_buffer.size() << std::endl;
        db_fout << "cycle: " << cnt_cycle << " dirty_buffer_size: " << m_buffer_size << " want_dirty_buffer_size: " << m_dirty_buffer.at(2).size() << " schedule_queue_size: " << m_schedule_queue.size() << std::endl;
        bp_fout << "cycle: " << cnt_cycle << " bp_pkt_flow: " << resubmit_pkt.m_key << " seq id: " << (int)resubmit_pkt.get_seq_id() << std::endl;
        if (!incoming_pkt.is_empty()) {
            incoming_buffer_rcv++;
            m_incoming_buffer.enqueue(std::move(incoming_pkt)); 
        }
        bool incoming_packet_valid { false };
        if (const auto& pkt { m_incoming_buffer.front() }; !pkt.is_empty()) {
            if (m_dirty_cam.test(pkt.m_key)) {
                if (m_buffer_size < N) {
                    incoming_buffer_to_dirty_buffer++;
                    enqueue(m_incoming_buffer.dequeue());
                }
            } else {
                incoming_packet_valid = true;    
            }
        }
        if (!resubmit_pkt.is_empty()) {
            if (resubmit_pkt.get_seq_id() ==  last_seq_id(m_seq_id_marker.get_seq_id(resubmit_pkt.m_key))) {
                // std::cout << "seq get" << std::endl;
                if (!m_schedule_cam.test(resubmit_pkt.m_key)) {
                    if (!m_dirty_buffer.at(resubmit_pkt.m_key).is_empty()) {
                        m_schedule_queue.enqueue(resubmit_pkt.m_key);
                        m_schedule_cam.set(resubmit_pkt.m_key);
                    }
                    else {
                       
                        m_dirty_cam.reset(resubmit_pkt.m_key);
                    }
                }
            }
            return resubmit_pkt;
        } else {
            auto scheduled_pkt { schedule() };
            if (!scheduled_pkt.is_empty()) {
                return m_seq_id_marker.next(std::move(scheduled_pkt));
            } else if (incoming_packet_valid) {
                incoming_buffer_to_pipeline++;
                return m_seq_id_marker.next(m_incoming_buffer.dequeue());
            }
        }
        return Packet {};
    }

    void print_info() {
        std::cout << "incoming_buffer rcv: " << incoming_buffer_rcv << " to dirty buffer: " << incoming_buffer_to_dirty_buffer << " to pipeline: " << incoming_buffer_to_pipeline  << std::endl;
        std::cout << "dirty_buffer rcv " << dirty_buffer_rcv << " snd: " << dirty_buffer_snd << std::endl;
    }
};