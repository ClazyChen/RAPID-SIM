export module rapid.ZipfDistribution;

import std;

const bool use_true_zipf { false };

export template <size_t K = 2> class ZipfDistribution {
    constexpr const static size_t m_flow_count { K - 1 };
    double m_alpha { use_true_zipf ? 1.01 : 1.0 };
    std::array<double, m_flow_count> m_probabilities;
    double m_sum { 0.0 };

    std::default_random_engine m_engine { std::random_device {}() };
    std::uniform_real_distribution<> m_distribution {};

    void initialize()
    {
        m_sum = 0.0;
        for (size_t i { 0 }; i < m_flow_count; ++i) {
            m_probabilities.at(i) = 1.0 / std::pow(i + 1, m_alpha);
        }
        m_sum = std::reduce(m_probabilities.begin(), m_probabilities.end(), 0.0);
        for (size_t i{ 1 }; i < m_flow_count; ++i) {
            m_probabilities.at(i) += m_probabilities.at(i - 1);
        }
        m_probabilities.at(m_flow_count - 1) = m_sum;
        if constexpr (use_true_zipf) {
            m_sum = std::max(m_sum, 1.0 / (m_alpha - 1));
        } 
        m_distribution = std::uniform_real_distribution<>(0.0, m_sum);
    }

public:
    ZipfDistribution()
    {
        initialize();
    }
    ZipfDistribution(double alpha)
        : m_alpha(alpha)
    {
        initialize();
    }

    unsigned short next()
    {
        double x { m_distribution(m_engine) };
        if (x > m_probabilities.back()) {
            return 0;
        }
        auto it { std::lower_bound(m_probabilities.begin(), m_probabilities.end(), x) };
        auto res { static_cast<unsigned short>(it - m_probabilities.begin() + 1) };
        return res;
    }
};



//FC_NUM 生成的流簇的数量
// 需要提前生成好1000000个包，按需求排列
export template <size_t K = 2, size_t FC_NUM = 1, size_t N = 1000000, size_t PKT_GAP = 10, size_t FIXED_FLOW_NUM = 10, size_t BURST_GAP = 1, int BURST_RATE = 20> 
class TunedZipfDistribution {
    constexpr const static size_t m_flow_count { K - 1 };
    size_t m_flow_cluster_num { FC_NUM };
    size_t m_pkt_gap { PKT_GAP };
    size_t m_burst_gap { BURST_GAP };
    double m_burst_rate{ (double) BURST_RATE / 100};
    double m_alpha{ use_true_zipf ? 1.01 : 1.0 };
    std::array<double, m_flow_count> m_probabilities;
    double m_sum{ 0.0 };

    std::default_random_engine m_engine { std::random_device {}() };
    std::uniform_real_distribution<> m_distribution {};

    std::array<size_t, N> m_packet_pool {};

    std::array<size_t, N> m_out_pkt_seq {};
    
    size_t cnt_pkt{ 0 };

    size_t m_fixed_flow_num{ FIXED_FLOW_NUM };
    std::ofstream fout;
    std::string zipf_filename;
    
    //void initialize_parameter(size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, double burst_rate) {
    //    m_flow_cluster_num = fc_num;
    //    m_fixed_flow_num = fixed_flow_num;
    //    m_pkt_gap = pkt_gap;
    //    m_burst_gap = burst_gap;
    //    m_burst_rate = burst_rate;
    //}
    
    void initialize()
    {
        //initialize_parameter(fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_rate);
        std::ifstream zipf_in;
        zipf_filename = std::format("zipf_{}_{}_{}_{}_{}.txt", m_flow_cluster_num, m_fixed_flow_num, m_pkt_gap, m_burst_gap, (int)(m_burst_rate * 100));
        zipf_in.open(zipf_filename, std::ios_base::in);
        if (zipf_in.good()) {
            std::string temp;
            int cnt = 0;
            while (getline(zipf_in, temp)) {
                std::istringstream ss(temp);
                ss >> m_out_pkt_seq[cnt];
                cnt++;
            }
            std::cout << "zipf trace input" << std::endl;
        }
        else {
            std::cout << "initialize zipf " << m_flow_cluster_num << " " << m_fixed_flow_num << " " << m_pkt_gap << " " << m_burst_gap << " " << m_burst_rate << std::endl;
            m_sum = 0.0;
            for (size_t i{ 0 }; i < m_flow_count;) {
                //m_probabilities.at(i) = 1.0 / std::pow(i + 1, m_alpha);
                for (size_t j{ 0 }; j < m_flow_cluster_num && i < m_flow_count; ++j, ++i) {
                    m_probabilities.at(i) = use_true_zipf ? 1.0 / std::pow(i / m_flow_cluster_num + 1, m_alpha) : 1.0 / (i / m_flow_cluster_num + 1);
                }
            }
            m_sum = std::reduce(m_probabilities.begin(), m_probabilities.end(), 0.0);
            for (size_t i{ 1 }; i < m_flow_count; ++i) {
                m_probabilities.at(i) += m_probabilities.at(i - 1);
            }
            m_probabilities.at(m_flow_count - 1) = m_sum;
            if constexpr (use_true_zipf) {
                m_sum = std::max(m_sum, 1.0 / (m_alpha - 1));
            }
            m_distribution = std::uniform_real_distribution<>(0.0, m_sum);
            for (int i = 0; i < N; i++) {
                double x{ m_distribution(m_engine) };
                if (x > m_probabilities.back()) {
                    i--;
                    continue;
                }
                auto it{ std::lower_bound(m_probabilities.begin(), m_probabilities.end(), x) };
                auto flow_id = static_cast<unsigned short>(it - m_probabilities.begin() + 1);
                m_packet_pool[i] = flow_id;
            }
            std::cout << "packet pool generated" << std::endl;
            std::array<int, K> m_flow_pkt_cnt {};
            for (int i = 0; i < N; i++) {
                m_flow_pkt_cnt[m_packet_pool[i]] += 1;
            }

            //packet pool中，id为1到fixed_flow_num的流为大流，让这些流的数据包间隔相等，给这些流的包分配位置
            for (int big_flow_id = 1; big_flow_id < m_fixed_flow_num + 1; big_flow_id++) {
                auto pkt_num = m_flow_pkt_cnt[big_flow_id];
                auto burst_pkt_num = (int)(pkt_num * m_burst_rate); //突发到达的包数（和前一个包间隔很短到达）
                auto normal_pkt_num = pkt_num - burst_pkt_num;
                auto burst_start = (pkt_num - burst_pkt_num) / 2;
                auto burst_end = burst_start + burst_pkt_num;
            
                //auto flow_len = pkt_num + (pkt_num - 1) * m_pkt_gap;
                auto flow_len = pkt_num + burst_pkt_num * m_burst_gap + (normal_pkt_num - 1) * m_pkt_gap;
                auto pos_distribution = std::uniform_int_distribution<>(0, N - flow_len);
                int x{ pos_distribution(m_engine) };
                while (m_out_pkt_seq[x] != 0) {
                    x = pos_distribution(m_engine);
                }
                auto index = x;
                for (int i = 0; i < pkt_num; i++) {
                    while (m_out_pkt_seq[index] != 0) {
                        index = (index + 1) % N; // 这里就没办法严格保证包的间隔相等了
                    }
                    m_out_pkt_seq[index] = big_flow_id;
                    if (burst_start <= i  && i <= burst_end - 1) {
                        index = (index + (m_burst_gap + 1)) % N;
                    }
                    else {
                        index = (index + (m_pkt_gap + 1)) % N;
                    }
                }
            }
            //为剩余的流的包分配位置
            int pos_num = 0;
            for (int i = 0; i < N; i++) {
                if (m_packet_pool[i] >= m_fixed_flow_num + 1) {
                    while (m_out_pkt_seq[pos_num] != 0) {
                        pos_num++;
                        if (pos_num >= N) {
                            std::cout << "pkt_num exceed limit" << std::endl;
                        }
                    }
                    m_out_pkt_seq[pos_num] = m_packet_pool[i];
                }
            }
            std::cout << "packet sequence inited" << std::endl;
            //std::ofstream fout;
            //fout.open("./packet_seq.txt", std::ios_base::out);
            //for (int i = 0; i < N; i++) {
            //    fout << m_out_pkt_seq[i] << std::endl;
            //}
        
            fout.open(zipf_filename, std::ios_base::out);
            for (int i = 0; i < N; i++) {
                fout << m_out_pkt_seq[i] << std::endl;
            }
        }

        //std::ofstream fout;
        //fout.open("./packet_seq.txt", std::ios_base::out);
        //for (int i = 0; i < N; i++) {
        //    fout << m_out_pkt_seq[i] << std::endl;
        //}
    }

public:
    TunedZipfDistribution()
    {
        //initialize();
    }
    TunedZipfDistribution(double alpha)
        : m_alpha(alpha)
    {
        //initialize();
    }
    TunedZipfDistribution(size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, double burst_rate) 
        : m_flow_cluster_num(fc_num), m_fixed_flow_num(fixed_flow_num), m_pkt_gap(pkt_gap), m_burst_gap(burst_gap), m_burst_rate(burst_rate) {
        //initialize();
    }

    void set_parameter_and_init(size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, double burst_rate) {
        m_flow_cluster_num = fc_num;
        m_fixed_flow_num = fixed_flow_num;
        m_pkt_gap = pkt_gap;
        m_burst_gap = burst_gap;
        m_burst_rate = burst_rate;
        initialize();
    }
    void set_parameter_and_init() {
        initialize();
    }

    void reset() {
        cnt_pkt = 0;
    }

    unsigned short next()
    {
        auto res = m_out_pkt_seq.at(cnt_pkt);
        cnt_pkt++;
        //std::ofstream fout;
        //fout.open("./packet_seq.txt", std::ios_base::out);
        //for (int i = 0; i < N; i++) {
        //    fout << m_out_pkt_seq[i] << std::endl;
        //}
        // fout << res << std::endl;
        return res;
    }
};