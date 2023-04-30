module;
#include <cstddef>
#include <iostream>

export module rapid.Packet;

export class Packet {
    static int s_id;

public:
    int m_id { 0 };
    short m_key { 0 };
    std::byte m_backward_tag_bitmap { 0 };
    std::byte m_write_back_bitmap { 0 };
    Packet() = default;
    Packet(short key)
        : m_id(++s_id)
        , m_key(key)
    {
    }
    Packet(short key, std::byte write_back_bitmap)
        : m_id(++s_id)
        , m_key(key)
        , m_write_back_bitmap(write_back_bitmap)
    {
    }
    constexpr bool is_empty() const
    {
        return m_key == 0;
    }
    constexpr bool is_backward_packet(std::byte dest_mask) const
    {
        return m_backward_tag_bitmap >= dest_mask;
    }
    constexpr bool is_write_back_packet(std::byte peer_mask) const
    {
        return (m_write_back_bitmap & peer_mask) != std::byte { 0 };
    }
    void set_backward_tag(std::byte bitmap)
    {
        m_backward_tag_bitmap |= bitmap;
    }
};

int Packet::s_id { 0 };

export std::ostream& operator<<(std::ostream& os, const Packet& pkt)
{
    os << "Packet { id: " << pkt.m_id << ", key: " << pkt.m_key << ", backward_tag_bitmap: " << static_cast<int>(pkt.m_backward_tag_bitmap) << ", write_back_bitmap: " << static_cast<int>(pkt.m_write_back_bitmap) << " }";
    return os;
}

export int g_clock { 0 };