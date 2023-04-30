module;
#include <initializer_list>

export module rapid.PacketGenerator;

import rapid.ZipfDistribution;
import rapid.GeometryDistribution;
import rapid.WriteBackGenerator;
import rapid.Packet;

export template <size_t K = 2>
class PacketGenerator {
    ZipfDistribution<K> m_zipf; // to control the key
    GeometryDistribution m_geo; // to control the flow
    WriteBackGenerator m_write_back_generator;

    int m_clock { 0 };

public:
    PacketGenerator() = default;
    PacketGenerator(double lambda, double alpha = 1.0)
        : m_zipf(alpha)
        , m_geo(lambda)
    {
    }

    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l)
    {
        m_write_back_generator.initialize(l);
    }

    Packet next()
    {
        if (m_clock == 0) {
            m_clock = m_geo.next() - 1;
            return m_write_back_generator.set_write_back(Packet { m_zipf.next() });
        } else {
            --m_clock;
            return Packet {};
        }
    }
};