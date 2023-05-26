export module rapid.experimental;

export import rapid.experimental.RawPipeline;
export import rapid.experimental.SinglePeer;
export import rapid.experimental.OverlapPeer;

import rapid.Packet;
import rapid.Device;
import rapid.PacketGenerator;
import rapid.PacketAnalyzer;
import std;

export template <typename DeviceType, size_t K = 2>
    requires std::is_base_of_v<Device, DeviceType>
class Experiment {
    constexpr const static int m_extra_cycle_count = 128000;

    PacketGenerator<K> m_packet_generator;
    //SimplePacketGenerator<K> m_packet_generator;
    PacketAnalyzer<K> m_packet_analyzer;
    DeviceType m_device;
    size_t total_elapse_time{ 0 };
    std::ofstream elapse_info_out;

    void receive_packet(Packet&& pkt)
    {
        ++g_clock;
        if (!pkt.is_empty()) {
            pkt.set_timestamp();
            //std::cout << g_clock << " I " << pkt << std::endl;
            ++m_rx_packet_count;
        }
        pkt = m_device.next(std::move(pkt));
        if (!pkt.is_empty()) {
            total_elapse_time += pkt.get_time_offset();
            //std::cout << g_clock << " O " << pkt << std::endl;
            elapse_info_out << "packet num: " << m_tx_packet_count << " elapse time: " << pkt.get_time_offset() << std::endl;
            ++m_tx_packet_count;
        }
        m_packet_analyzer.receive_packet(std::move(pkt));
    }

    int m_rx_packet_count { 0 };
    int m_tx_packet_count { 0 };
    int m_target_count { 0 };

    void run_extra_cycles()
    {
        for (int i { 0 }; i < m_extra_cycle_count; ++i) {
            receive_packet(Packet {});
        }
    }

public:
    Experiment() 
    {
        m_device.initialize();
        elapse_info_out.open("./time_elapse.txt", std::ios_base::out);
    }
    Experiment(double lambda, double alpha = 1.01)
        : m_packet_generator(lambda, alpha)
    {
        m_device.initialize();
        elapse_info_out.open("./time_elapse.txt", std::ios_base::out);
    }

    Experiment(size_t front_buffer_size, size_t pir_type) {

    }

    void set_lambda(double lambda) {
        m_packet_generator.set_lambda(lambda);
    }

    void set_arr_period(size_t arr_period) {
        m_packet_generator.set_arr_period(arr_period);
    }

    void initialize_write_back_generator(std::initializer_list<std::pair<int, double>> l)
    {
        m_packet_generator.initialize_write_back_generator(l);
    }

    void initialize_write_back_generator(size_t wb_gap) {
        m_packet_generator.initialize_write_back_generator(wb_gap);
    }

    void initialize_zipf(size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, double burst_rate) {
        m_packet_generator.initialize_zipf(fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_rate);
    }

    void initialize_zipf() {
        m_packet_generator.initialize_zipf();
    }

    void run(int cycle_count)
    {
        for (int i { 0 }; i < cycle_count; ++i) {
            receive_packet(m_packet_generator.next());
        }
        run_extra_cycles();
    }

    void run_until(int packet_count)
    {
        m_target_count = packet_count;
        while (m_packet_generator.m_tx_packet_count < packet_count) {
            receive_packet(m_packet_generator.next());
        }
        run_extra_cycles();
    }

    void report(std::ostream& os) const
    {
        os << std::format("Recv Pkt = {}", m_rx_packet_count) << std::endl;
        os << std::format("Send Pkt = {}", m_tx_packet_count) << std::endl;
        //os << std::format("Wrng Pkt = {}", m_packet_analyzer.get_wrong_order_count()) << std::endl;
        //os << std::format("Wrong %  = {:.6f}", static_cast<double>(m_packet_analyzer.get_wrong_order_count()) / m_target_count) << std::endl;
        os << std::format("Drop  %  = {:.6f}", static_cast<double>(m_rx_packet_count - m_tx_packet_count) / m_target_count) << std::endl;
        os << std::format("average elapse time = {}", total_elapse_time / m_tx_packet_count) << std::endl;
        //os << std::format("FSize    = {}", m_device.FrontBufferSize()) << std::endl;
    }

    void reset() {
        m_rx_packet_count = 0;
        m_tx_packet_count = 0;
        m_target_count = 0;
        m_packet_analyzer.reset();
        m_packet_generator.reset();
        m_device.reset();
    }

    void print_info() {
        m_device.print_info();
    }
};