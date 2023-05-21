export module rapid.PacketGenerator;

import rapid.ZipfDistribution;
import rapid.GeometryDistribution;
import rapid.WriteBackGenerator;
import rapid.Packet;
import std;

//export template <size_t K = 2>
//class PacketGenerator {
//    ZipfDistribution<K> m_zipf; // to control the key
//    GeometryDistribution m_geo; // to control the flow
//    WriteBackGenerator m_write_back_generator;
//
//    int m_clock { 0 };
//
//public:
//    int m_tx_packet_count { 0 };
//
//    PacketGenerator() = default;
//    PacketGenerator(double lambda, double alpha = 1.01)
//        : m_zipf(alpha)
//        , m_geo(lambda)
//    {
//    }
//
//    void set_lambda(double lambda)
//    {
//        m_geo.set_lambda(lambda);
//    }
//
//    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l)
//    {
//        m_write_back_generator.initialize(l);
//    }
//
//    Packet next()
//    {
//        if (m_clock == 0) {
//            m_clock = m_geo.next() - 1;
//            ++m_tx_packet_count;
//            return m_write_back_generator.set_write_back(Packet { m_zipf.next() });
//        } else {
//            --m_clock;
//            return Packet {};
//        }
//    }
//};

export template <size_t K = 2>
class PacketGenerator {
    TunedZipfDistribution<K, 1, 100000, 10, 1, 5, 20> m_zipf; // to control the key
    //ZipfDistribution<K> m_zipf;
    GeometryDistribution m_geo; // to control the flow
    FixedWriteBackGenerator<K> m_write_back_generator;
    //WriteBackGenerator m_write_back_generator;
    int m_clock{ 0 };
    int m_arrive_period{ 10 }; //每几个周期到达一个包
    std::ofstream fout;

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

    void set_arr_period(size_t arr_period) {
        m_arrive_period = arr_period;
    }

    void initialize_write_back_generator(size_t wb_gap)
    {
        m_write_back_generator.initialize(wb_gap);
    }

    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l) {
        m_write_back_generator.initialize(l);
    }

    void reset() {
        
        //fout.open("./packet_seq.txt", std::ios_base::out);
        m_zipf.reset();
    }

    Packet next()
    {
        //if (m_clock == 0) {
        //    m_clock = m_geo.next() - 1;
        //    ++m_tx_packet_count;
        //    return m_write_back_generator.set_write_back(Packet{ m_zipf.next() });
        //}
        //else {
        //    --m_clock;
        //    return Packet{};
        //}

        if (m_clock == 0) {
            m_clock = m_arrive_period - 1;
            ++m_tx_packet_count;
            auto flow_id = m_zipf.next();
            auto pkt = m_write_back_generator.set_write_back(Packet{ flow_id });
            //fout << flow_id << " " << (int)pkt.m_write_back_bitmap<< std::endl;
            return pkt;
        }
        else {
            --m_clock;
            //fout << 0 << std::endl;
            return Packet{};
        }
    }
};

export template <size_t K = 2>
class SimplePacketGenerator {
public:
    int m_tx_packet_count { 0 };
    SimplePacketGenerator() = default;
    SimplePacketGenerator(double lambda, double alpha = 1.01)
    {
    }

    void set_lambda(double lambda)
    {
    }

    void set_arr_period(size_t arr_period) {
    }

    void initialize_write_back_generator(size_t wb_gap)
    {
    }

    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l) {
    }

    void reset() {
    }
    Packet next()
    {
        //if (m_clock == 0) {
        //    m_clock = m_arrive_period - 1;
        //    ++m_tx_packet_count;
        //    auto flow_id = m_zipf.next();
        //    auto pkt = m_write_back_generator.set_write_back(Packet{ flow_id });
        //    //fout << flow_id << " " << (int)pkt.m_write_back_bitmap<< std::endl;
        //    return pkt;
        //}
        //else {
        //    --m_clock;
        //    //fout << 0 << std::endl;
        //    return Packet{};
        //}
        ++m_tx_packet_count;
        unsigned short flow_id = 1;
        auto pkt = Packet{ flow_id };
        pkt.m_write_back_bitmap |= std::byte(1);
        return pkt;
    }
};
