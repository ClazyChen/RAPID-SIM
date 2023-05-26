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
    TunedZipfDistribution<K, 1, 100000, 10, 2, 5, 20> m_zipf; // to control the key
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

    void initialize_zipf(size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, double burst_rate) {
        m_zipf.set_parameter_and_init(fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_rate);
    }

    void initialize_zipf() {
        m_zipf.set_parameter_and_init();
    }

    void reset() {
        
        fout.open("./packet_seq.txt", std::ios_base::out);
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
            //m_clock = m_arrive_period - 1;
            m_clock = m_geo.next() - 1;
            ++m_tx_packet_count;
            auto flow_id = m_zipf.next();
            auto pkt = m_write_back_generator.set_write_back(Packet{ flow_id });
            fout << flow_id << " " << (int)pkt.m_write_back_bitmap<< std::endl;
            return pkt;
        }
        else {
            --m_clock;
            fout << 0 << std::endl;
            return Packet{};
        }
    }
};

export template <size_t K = 2>
class SimplePacketGenerator {
    FixedWriteBackGenerator<K> m_write_back_generator;
    std::ofstream fout;
public:
    int m_tx_packet_count { 0 };
    int wb_gap{ 1 };
    int m_clock{ 0 };
    int m_arr_period{ 10 };
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
        fout.open("./packet_seq.txt", std::ios_base::out);
    }
    Packet next()
    {
        //if (m_clock == 0) {
        //    m_clock = m_arrive_period - 1;
        //    ++m_tx_packet_count;
        //    auto flow_id = m_zipf.next();
        //    auto pkt = m_write_back_generator.set_write_back(packet{ flow_id });
        //    //fout << flow_id << " " << (int)pkt.m_write_back_bitmap<< std::endl;
        //    return pkt;
        //}
        //else {
        //    --m_clock;
        //    //fout << 0 << std::endl;
        //    return packet{};
        //}
        //std::cout << m_arr_period << std::endl;
        if (m_clock == 0) {
            m_clock = m_arr_period - 1;
            ++m_tx_packet_count;
            //std::cout << m_write_back_generator.m_wb_gap << std::endl;
            auto pkt = m_write_back_generator.set_write_back(Packet{ 1 });
            fout<< 1 << " " << (int)pkt.m_write_back_bitmap << std::endl;
            return pkt;
        }
        else {
            m_clock--;
            fout << 0 << std::endl;
            return Packet{};
        }
    }
};
