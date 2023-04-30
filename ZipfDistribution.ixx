module;
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <array>

export module rapid.ZipfDistribution;

export template <size_t K = 2> class ZipfDistribution {
    constexpr const static size_t m_flow_count { K - 1 };
    double m_alpha { 1.0 };
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

    short next()
    {
        double x { m_distribution(m_engine) };
        auto it = std::lower_bound(m_probabilities.begin(), m_probabilities.end(), x);
        return static_cast<short>(it - m_probabilities.begin() + 1);
    }
};