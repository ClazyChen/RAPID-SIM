export module rapid.GeometryDistribution;
import std;

export class GeometryDistribution {
    double m_lambda { 0.5 };

    std::default_random_engine m_engine { std::random_device {}() };
    std::uniform_real_distribution<> m_distribution { 0, 1 };

public:
    GeometryDistribution() = default;
    GeometryDistribution(double lambda)
        : m_lambda(lambda)
    {
    }

    void set_lambda(double lambda) { m_lambda = lambda; }

    int next()
    {
        if (m_lambda == 1.0) {
            return 1;
        } else {
            double x { m_distribution(m_engine) };
            return static_cast<int>(std::ceil(std::log(1 - x) / std::log(1 - m_lambda)));
        }
    }
};