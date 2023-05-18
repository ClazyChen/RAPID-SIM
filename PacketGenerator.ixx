module;
#include <initializer_list>

export module rapid.PacketGenerator;

import rapid.ZipfDistribution;
import rapid.GeometryDistribution;
import rapid.WriteBackGenerator;
import rapid.Packet;

export template <size_t K = 2, size_t zipf_N = 3>
class PacketGenerator {
    //生成 zipf_num个zipf分布，每次取包的时候从这些zipf分布中取
    ZipfDistribution<K, zipf_N> m_zipf; // to control the key
    GeometryDistribution m_geo; // to control the flow
    WriteBackGenerator m_write_back_generator;

    int m_clock { 0 };

public:
    int m_tx_packet_count { 0 };

    PacketGenerator() = default;
    PacketGenerator(double lambda, double alpha = 1.01)
        : m_zipf(alpha)
        , m_geo(lambda)
    {
    }

    void set_lambda(double lambda)
    {
        m_geo.set_lambda(lambda);
    }

    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l)
    {
        m_write_back_generator.initialize(l);
    }

    Packet next()
    {
        if (m_clock == 0) {
            m_clock = m_geo.next() - 1;
            ++m_tx_packet_count;
            return m_write_back_generator.set_write_back(Packet { m_zipf.next() });
        } else {
            --m_clock;
            return Packet {};
        }
    }
};