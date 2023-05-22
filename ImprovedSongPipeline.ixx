export module rapid.ImprovedSongPipeline;

import rapid.Packet;
import rapid.Device;
import rapid.PacketQueue;
import rapid.SongReadWritePeer;
import std;

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4, size_t FRONT_BUFFER_SIZE = 1024>
class ImprovedSongPipeline : public Device {
    PacketQueue<FRONT_BUFFER_SIZE> m_front;
    SongReadWritePeer<N, 0, PROC_NUM - 1, K> m_peer;
    std::ofstream front_buffer_info_out;

    size_t cnt_cycle{ 0 };


public:
    size_t front_buffer_cnt_rcv{ 0 };
    size_t front_buffer_cnt_snd{ 0 };
    size_t peer_cnt_rcv{ 0 };
    size_t peer_cnt_snd{ 0 };

    ImprovedSongPipeline() = default;
    void initialize()
    {
        front_buffer_info_out.open("./fb_size.txt", std::ios_base::out);
        // no action
    }
    Packet next(Packet&& pkt) override
    {
        if (!pkt.is_empty()) {
            front_buffer_cnt_rcv++;
        }
        front_buffer_info_out << "cycle: " << cnt_cycle++ << " front size: " << m_front.size() << std::endl;
        if (!pkt.is_empty()) {
            m_front.enqueue(std::move(pkt));
        }
        Packet front_pkt {};
        if (!m_peer.is_full()) {
            front_pkt = m_front.dequeue();
        }
        if (!front_pkt.is_empty()) {
            front_buffer_cnt_snd++;
            peer_cnt_rcv++;
        }
        auto peer_pkt = m_peer.next(std::move(front_pkt));
        if (!peer_pkt.is_empty()) {
            peer_cnt_snd++;
        }
        return peer_pkt;
    }

    void print_info() {
        std::cout << "front_buffer rcv: " << front_buffer_cnt_rcv << " snd: " << front_buffer_cnt_snd << std::endl;
        std::cout << "wr_peer rcv " << peer_cnt_rcv << " snd: " << peer_cnt_snd << std::endl;
        m_peer.print_info();
    }
};