module;
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <array>

export module rapid.ZipfDistribution;

const bool use_true_zipf { false };

export template <size_t K = 2, size_t zipf_N = 3> class ZipfDistribution {
    //K 是流的总数，需要将这些流分成zipf_N份，每份都满足一个zipf分布
    constexpr const static size_t m_flow_count { K - 1 };
    double m_alpha { use_true_zipf ? 1.01 : 1.0 };
    std::array<double, m_flow_count> m_probabilities;
    double m_sum { 0.0 };

    std::default_random_engine m_engine { std::random_device {}() };
    std::uniform_real_distribution<> m_distribution {};

    void initialize()
    {
        size_t each_zipf_flows_num = m_flow_count / zipf_N;
        m_sum = 0.0;
        for (size_t j{ 0 }; j < zipf_N; ++j) {        
            if (j != zipf_N - 1) {
                for (size_t i{ each_zipf_flows_num * j }; i < each_zipf_flows_num * (j + 1) - 1; ++i) {
                    m_probabilities.at(i) = 1.0 / std::pow(i - each_zipf_flows_num * j + 1, m_alpha);
                }
            }
            else {
                for (size_t i{ each_zipf_flows_num * j }; i < m_flow_count; ++i) {
                    m_probabilities.at(i) = 1.0 / std::pow(i - each_zipf_flows_num * j + 1, m_alpha);
                }
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