module;
#include <array>

export module rapid.SeqIdMarker;

import rapid.Packet;
import rapid.Device;

export template <size_t K = 2>
class SeqIdMarker : public Device {
    std::array<std::byte, K> m_seq_id {};

public:
    SeqIdMarker() = default;
    Packet next(Packet&& pkt) override {
        if (pkt.is_empty() || pkt.get_seq_id() != std::byte(0)) {
            return pkt;
        }
        else {
            auto m_seq { m_seq_id[pkt.m_key] };
            m_seq_id[pkt.m_key] = next_seq_id(m_seq);
            pkt.set_seq_id(m_seq);
            return pkt;
        }
    }
    std::byte get_nxt_seq_id(Packet pkt) {
        return m_seq_id[pkt.m_key];
    }
};