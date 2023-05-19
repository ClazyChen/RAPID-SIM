export module rapid.SongPipeline;

import rapid.Packet;
import rapid.PacketQueue;
import rapid.Device;
import rapid.VirtualPipeline;
import rapid.SeqIdMarker;
import rapid.SongPiw;
import std;

const size_t PROC_CYCLES { 15 };
consteval size_t PIPELINE_LENGTH(size_t IID, size_t OID) {
    return (OID - IID + 1)* PROC_CYCLES;
}
consteval size_t BACKBUS_LENGTH(size_t IID, size_t OID) {
    return 2 + (OID - IID) * 2;
}
consteval size_t CLOCK_MAX(size_t IID, size_t OID) {
    return PIPELINE_LENGTH(IID, OID) + BACKBUS_LENGTH(IID, OID);
}

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4>
class SongPipeline : public Device {
    constexpr const static size_t m_proc_num { PROC_NUM };
    constexpr const static size_t m_pipeline_length { PIPELINE_LENGTH(1, PROC_NUM) };
    constexpr const static size_t m_backbus_length { BACKBUS_LENGTH(1, PROC_NUM) };
    constexpr const static size_t m_clock_max { CLOCK_MAX(1, PROC_NUM) };

    PacketQueue<N> m_front_buffer;
    SeqIdMarker<K> m_seq_id_marker;
    VirtualPipeline<m_pipeline_length> m_pipeline;
    VirtualPipeline<m_backbus_length> m_backbus;
    SongPiw<std::byte(1), m_clock_max, K> m_piw;

    Packet m_temp_pkt;

public:
    SongPipeline() = default;
    void initialize() {
        // no action
    }
    Packet next(Packet&& pkt) override {
        if (!pkt.is_empty()) {
            m_front_buffer.enqueue(std::move(pkt));
        }
        Packet next_pkt { std::move(m_temp_pkt) };
        if (next_pkt.is_empty()) {
            next_pkt = m_front_buffer.dequeue();
        }
        if (!next_pkt.is_empty()) {
            if constexpr (OUTPUT) {
                std::cout << "    " << g_clock << " "
                          << "next_pkt is " << next_pkt << std::endl;
            }
        }
        auto seq_pkt { m_seq_id_marker.next(std::move(next_pkt)) };
        auto pipe_pkt { m_pipeline.next(std::move(seq_pkt)) };
        auto [bp, pp] { m_piw.next(std::move(pipe_pkt)) };
        m_temp_pkt = m_backbus.next(std::move(bp));
        return std::move(pp);
    }

    int FrontBufferSize() const {
        return m_front_buffer.size();
    }

};