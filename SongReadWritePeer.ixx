export module rapid.SongReadWritePeer;

import rapid.Packet;
import rapid.Device;
import rapid.SongPir;
import rapid.SongPiw;
import rapid.VirtualPipeline;
import rapid.SongPipeline;
import rapid.PacketQueue;
import std;
#include "assert.h"
const size_t FRONT_CYCLES { 5 };
//export const size_t PROC_CYCLES { 15 };
//export const size_t BACKBUS_CYCLES { 2 };
//export consteval size_t PIPELINE_LENGTH(size_t IID, size_t OID)
//{
//    return (OID - IID + 1) * PROC_CYCLES;
//}
//export consteval size_t BACKBUS_LENGTH(size_t IID, size_t OID)
//{
//    return 2 + (OID - IID) * BACKBUS_CYCLES;
//}
//export consteval size_t CLOCK_MAX(size_t IID, size_t OID)
//{
//    return PIPELINE_LENGTH(IID, OID) + BACKBUS_LENGTH(IID, OID);
//}

export template <size_t N, size_t RID, size_t WID, size_t K = 2>
class SongReadWritePeer : public Device {
    constexpr const static size_t m_pipeline_length { PIPELINE_LENGTH(RID, WID) - FRONT_CYCLES };
    constexpr const static size_t m_backbus_length { BACKBUS_LENGTH(RID, WID) };
    constexpr const static size_t m_clock_max { CLOCK_MAX_2(RID, WID) - FRONT_CYCLES };

    VirtualPipeline<FRONT_CYCLES> m_front;
    VirtualPipeline<m_pipeline_length> m_pipeline;
    VirtualPipeline<m_backbus_length> m_backbus;
    VirtualPipeline<m_backbus_length> m_wb_bus;
    SongPir<N, K> m_pir;
    SongPiw<std::byte(1 << RID), m_clock_max, K> m_piw;
    Packet m_temp_pkt;
    Packet m_wb_pkt;
    std::ofstream w_r_peer_out;
    size_t cnt_cycle{ 0 };


public:
    SongReadWritePeer() {
        w_r_peer_out.open("./wr_info.txt", std::ios_base::out);
        //std::cout << "pipeline length " << PIPELINE_LENGTH(RID, WID) << " back bus length " << BACKBUS_LENGTH(RID, WID) << "clock max " << CLOCK_MAX_2(RID, WID) << std::endl;
    };

    bool is_full() const {
        return m_pir.is_full();
    }

    Packet next(Packet&& pkt) override {
        //assert(0);
        cnt_cycle++;
        //如果pir的 buffer满了， front_pipeline需要停顿
        auto front_pkt = m_pir.is_full() ? Packet {} : m_front.next(std::move(pkt));
        m_pir.write_cam(std::move(m_wb_pkt));
        auto pir_pkt{ m_pir.next(std::move(front_pkt), std::move(m_temp_pkt)) };
        auto pipe_pkt{ m_pipeline.next(std::move(pir_pkt)) };
        auto [bp_wb, pp] { m_piw.next(std::move(pipe_pkt)) };
        auto [bp, wb] { bp_wb };
        w_r_peer_out << "cycle: " << cnt_cycle << " w_bp_pkt: " << bp.m_key << " w_wb_pkt: " << wb.m_key << " pp_pkt: " << pp.m_key ;
        m_temp_pkt = m_backbus.next(std::move(bp));
        m_wb_pkt = m_wb_bus.next(std::move(wb));
        w_r_peer_out << " r_bp_pkt: " << m_temp_pkt.m_key << "  r_wb_pkt: " << m_wb_pkt.m_key << std::endl;
        return std::move(pp);
    }

    void print_info() {
        m_pir.print_info();
    }
};;