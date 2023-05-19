export module rapid.ReadWritePeer;

import rapid.Packet;
import rapid.Pir;
import rapid.Piw;
import rapid.FrontScheduler;
import rapid.VirtualPipeline;
import rapid.Device;
import std;

export const size_t FB_PIR { 5 };
export const size_t PIR_PIW { 10 };
export consteval size_t RING_LENGTH(size_t RID, size_t WID)
{
    return 2 + (WID - RID) * 2;
}
export consteval size_t CLOCK_MAX(size_t RID, size_t WID)
{
    return PIR_PIW + (PIR_PIW + FB_PIR) * (WID - RID) + RING_LENGTH(RID, WID);
}

export template <size_t RID, size_t WID, size_t N, size_t K = 2, size_t EXPLICIT_CLOCK_MAX = 0, bool ENABLE_UNWRITEABLE = false>
    requires(RID < WID)
class ReadWritePeer : public DualPortDevice {
    constexpr const static std::byte m_pir_mask { std::byte(1 << RID) };
    constexpr const static std::byte m_piw_mask { std::byte(1 << WID) };
    constexpr const static unsigned short m_ring_length { RING_LENGTH(RID, WID) };
    constexpr const static unsigned short m_clock_max { EXPLICIT_CLOCK_MAX ? EXPLICIT_CLOCK_MAX : CLOCK_MAX(RID, WID) };

    Pir<m_pir_mask, N, m_clock_max, K> m_pir;
    Piw<m_pir_mask, m_ring_length, K, ENABLE_UNWRITEABLE> m_piw;
    FrontScheduler<N> m_front_scheduler;
    VirtualPipeline<FB_PIR> m_fb_pir;

    Packet temp_backward_packet {};
    Packet temp_write_back_packet {};
    unsigned short temp_key {};

public:
    ReadWritePeer() = default;

    void reset() override {
        m_pir.reset();
    }

    std::pair<int, int> get_drop_packet_count() const {
        return { m_front_scheduler.m_drop_packet_count, m_pir.m_drop_packet_count };
    }

    Packet next1(Packet&& pir) override
    {
        auto [vp_in, key] = m_front_scheduler.next(std::move(pir), std::move(temp_backward_packet));
        m_pir.count_backward_packet(key);
        auto vp_out = m_fb_pir.next(std::move(vp_in));
        m_pir.write_cam(std::move(temp_write_back_packet));
        auto [pir_out, cd] = m_pir.next(std::move(vp_out));
        temp_key = cd;
        return pir_out;
    }

    unsigned short get_lock_key() override {
        return m_pir.get_scheduling_key();
    }

    Packet next2(Packet&& piw) override
    {
        m_piw.cancel_dirty_step(temp_key);
        auto [piw_out, bp, wb] = m_piw.next(std::move(piw));
        temp_backward_packet = bp;
        temp_write_back_packet = wb;
        return piw_out;
    }

    void unlock_key(unsigned short key) override {
        m_piw.reset_scheduling(key);
    }
};
