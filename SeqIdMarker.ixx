export module rapid.SeqIdMarker;

import rapid.Packet;
import rapid.Device;
import std;

export template <size_t K = 2>
class SeqIdMarker : public Device {
    std::array<std::byte, K> m_seq_id {};

public:
    SeqIdMarker() = default;

    std::byte get_seq_id(unsigned short key) const {
        return m_seq_id[key];
    }

    Packet next(Packet&& pkt) override {
        if (pkt.is_empty() || pkt.get_seq_id() != std::byte(0)) {
            return pkt;
        }
        else {
            m_seq_id[pkt.m_key] = next_seq_id(m_seq_id[pkt.m_key]);
            pkt.set_seq_id(m_seq_id[pkt.m_key]);
            return pkt;
        }
    }
};