export module rapid.ImprovedSongPipeline;

import rapid.Packet;
import rapid.Device;
import rapid.PacketQueue;
import rapid.SongReadWritePeer;
import std;

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4, size_t FRONT_BUFFER_SIZE = 8192>
class ImprovedSongPipeline : public Device {
    PacketQueue<FRONT_BUFFER_SIZE> m_front;
    SongReadWritePeer<N, 0, PROC_NUM - 1, K> m_peer;

public:
    ImprovedSongPipeline() = default;
    void initialize()
    {
        // no action
    }
    Packet next(Packet&& pkt) override
    {
        if (!pkt.is_empty()) {
            m_front.enqueue(std::move(pkt));
        }
        Packet front_pkt {};
        if (!m_peer.is_full()) {
            front_pkt = m_front.dequeue();
        }
        return m_peer.next(std::move(front_pkt));
    }
};