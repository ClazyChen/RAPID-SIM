export module rapid.SongReadWritePeer;

import rapid.Packet;
import rapid.Device;
import rapid.SongPir;
import rapid.SongPiw;
import rapid.VirtualPipeline;
import rapid.SongPipeline;
import rapid.PacketQueue;
import std;

const size_t FRONT_CYCLES { 5 };

export template <size_t N, size_t RID, size_t WID, size_t K = 2>
class SongReadWritePeer : public Device {
    constexpr const static size_t m_pipeline_length { PIPELINE_LENGTH(RID, WID) - FRONT_CYCLES };
    constexpr const static size_t m_backbus_length { BACKBUS_LENGTH(RID, WID) };
    constexpr const static size_t m_clock_max { CLOCK_MAX(RID, WID) - FRONT_CYCLES };

    VirtualPipeline<FRONT_CYCLES> m_front;
    VirtualPipeline<m_pipeline_length> m_pipeline;
    VirtualPipeline<m_backbus_length> m_backbus;
    SongPir<N, K> m_pir;
    SongPiw<std::byte(1 << RID), m_clock_max, K> m_piw;
    Packet m_temp_pkt;

public:
    SongReadWritePeer() = default;

    bool is_full() const {
        return m_pir.is_full();
    }

    Packet next(Packet&& pkt) override {
        auto front_pkt { m_front.next(std::move(pkt)) };
        auto pir_pkt { m_pir.next(std::move(front_pkt), std::move(m_temp_pkt)) };
        auto pipe_pkt { m_pipeline.next(std::move(pir_pkt)) };
        auto [bp, pp] { m_piw.next(std::move(pipe_pkt)) };
        m_temp_pkt = m_backbus.next(std::move(bp));
        return std::move(pp);
    }
};