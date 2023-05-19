export module rapid.ImprovedSongPipeline;

import rapid.Packet;
import rapid.Device;
import rapid.SongPir;
import rapid.SongPiw;
import rapid.VirtualPipeline;
import rapid.SongPipeline;
import std;

const size_t FRONT_CYCLES { 5 };

export template <size_t N, size_t K = 2, size_t PROC_NUM = 4>
class ImprovedSongPipeline : public Device {
    constexpr const static size_t m_proc_num { PROC_NUM };
    constexpr const static size_t m_pipeline_length { PIPELINE_LENGTH(1, PROC_NUM) - FRONT_CYCLES };
    constexpr const static size_t m_backbus_length { BACKBUS_LENGTH(1, PROC_NUM) };
    constexpr const static size_t m_clock_max { CLOCK_MAX(1, PROC_NUM) - FRONT_CYCLES };

    VirtualPipeline<FRONT_CYCLES> m_front;
    VirtualPipeline<m_pipeline_length> m_pipeline;
    VirtualPipeline<m_backbus_length> m_backbus;
    SongPir<N, K> m_pir;
    SongPiw<std::byte(1), m_clock_max, K> m_piw;

    Packet m_temp_pkt;

public:
    ImprovedSongPipeline() = default;
    void initialize()
    {
        // no action
    }
    Packet next(Packet&& pkt) override
    {
        auto front_pkt { m_front.next(std::move(pkt)) };
        auto pir_pkt { m_pir.next(std::move(front_pkt), std::move(m_temp_pkt)) };
        auto pipe_pkt { m_pipeline.next(std::move(pir_pkt)) };
        auto [bp, pp] { m_piw.next(std::move(pipe_pkt)) };
        m_temp_pkt = m_backbus.next(std::move(bp));
        return std::move(pp);
    }
};