export module rapid.SongPiw;

import rapid.Packet;
import std;

export template <std::byte PEER_MASK, size_t CLOCK_MAX, size_t K = 2>
class SongPiw {
    constexpr const static std::byte m_peer_mask { PEER_MASK };
    constexpr const static size_t m_clock_max { CLOCK_MAX };

    struct DirtyState {
        bool m_exist { false };
        bool m_dirty { false };
        std::byte m_seq_id {};
        int m_update_time {};
        DirtyState() = default;
    };

    std::array<DirtyState, K> m_dirty_state {};
    Packet m_pkt_to_write {};

public:
    SongPiw() = default;

    Packet get_pkt_to_write() const {
        return m_pkt_to_write;
    }

    std::pair<Packet, Packet> next(Packet&& pkt)
    {
        m_pkt_to_write = Packet {};
        if (pkt.is_empty()) {
            return { pkt, pkt };
        } else {
            auto& state { m_dirty_state[pkt.m_key] };
            if (state.m_update_time + m_clock_max < g_clock) {
                // expired
                state.m_exist = false;
            }
            if (!state.m_exist) {
                if (pkt.is_write_back_packet(m_peer_mask)) {
                    if constexpr (OUTPUT) {
                        std::cout << "WRITE = " << pkt << std::endl; 
                    }
                    state.m_exist = true;
                    state.m_dirty = true;
                    state.m_seq_id = next_seq_id(pkt.get_seq_id());
                    state.m_update_time = g_clock;
                    m_pkt_to_write = pkt;
                }
                return { Packet {}, std::move(pkt) };
            } else {
                if (state.m_dirty) {
                    state.m_update_time = g_clock;
                    if (auto seq_id { pkt.get_seq_id() }; seq_id == state.m_seq_id) {
                        state.m_dirty = false;
                    }
                } else {
                    if (auto seq_id { pkt.get_seq_id() }; seq_id == state.m_seq_id) {
                        state.m_seq_id = next_seq_id(state.m_seq_id);
                        if (pkt.is_write_back_packet(m_peer_mask)) {
                            state.m_update_time = g_clock;
                            state.m_dirty = true;
                        }
                        return { Packet {}, std::move(pkt) };
                    } else {
                        state.m_update_time = g_clock;
                    }
                }
                return { std::move(pkt), Packet {} };
            }
        }
    }
};