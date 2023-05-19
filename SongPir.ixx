export module rapid.SongPir;

import rapid.Packet;
import rapid.PacketQueue;
import rapid.SeqIdMarker;
import rapid.VirtualPipeline;
import rapid.RoundRobinQueue;
import std;

export template <size_t N, size_t K = 2>
class SongPir {
    PacketQueue<N> m_incoming_buffer;
    SeqIdMarker<K> m_seq_id_marker;
    std::bitset<K> m_dirty_cam;
    std::bitset<K> m_schedule_cam;
    std::array<PacketQueue<N>, K> m_dirty_buffer;
    int m_buffer_size { 0 };
    RoundRobinQueue<K> m_schedule_queue;

    Packet schedule() {
        auto key { m_schedule_queue.dequeue() };
        if (key == 0) {
            return Packet {};
        }
        auto pkt { m_dirty_buffer.at(key).dequeue() };
        --m_buffer_size;
        if (m_dirty_buffer.at(key).is_empty()) {
            m_dirty_cam.reset(key);
            m_schedule_cam.reset(key);
        } else {
            m_schedule_queue.enqueue(key);
        }
        return pkt;
    }

    void enqueue(Packet&& pkt) {
        if (m_buffer_size < N) {
            ++m_buffer_size;
            m_dirty_buffer.at(pkt.m_key).enqueue(std::move(pkt));
        } else {
            ++m_drop_packet_count;
        }
    }

public:
    int m_drop_packet_count { 0 };
    SongPir() = default;

    void write_cam(Packet&& pkt) {
        if (!pkt.is_empty()) {
            if (pkt.get_seq_id() != m_seq_id_marker.get_seq_id(pkt.m_key)) {
                m_dirty_cam.set(pkt.m_key);
                m_schedule_cam.reset(pkt.m_key);
            }
        }
    }

    // resubmit > schedule > incoming
    Packet next(Packet&& incoming_pkt, Packet&& resubmit_pkt) {
        if (!incoming_pkt.is_empty()) {
            m_incoming_buffer.enqueue(std::move(incoming_pkt)); 
        }
        bool incoming_packet_valid { false };
        if (const auto& pkt { m_incoming_buffer.front() }; !pkt.is_empty()) {
            if (m_dirty_cam.test(pkt.m_key)) {
                enqueue(m_incoming_buffer.dequeue());
            } else {
                incoming_packet_valid = true;    
            }
        }
        if (!resubmit_pkt.is_empty()) {
            if (resubmit_pkt.get_seq_id() == m_seq_id_marker.get_seq_id(resubmit_pkt.m_key)) {
                m_schedule_cam.set(resubmit_pkt.m_key);   
            }
            return resubmit_pkt;
        } else {
            auto scheduled_pkt { schedule() };
            if (!scheduled_pkt.is_empty()) {
                return m_seq_id_marker.next(std::move(scheduled_pkt));
            } else if (incoming_packet_valid) {
                return m_seq_id_marker.next(m_incoming_buffer.dequeue());
            }
        }
        return Packet {};
    }
};