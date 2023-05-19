export module rapid.experimental.bench;

import rapid.SongPipeline;
import rapid.ImprovedSongPipeline;
import rapid.experimental;
import std;

export template <size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t EXPLICIT_CLOCK_MAX>
class ExperimentBench {
public:
    ExperimentBench() = default;

    void run_all(std::ostream& os, int packet_number) {
        for (double lambda{ 0.1 }; lambda <= 0.9 + 1e-8; lambda += 0.1) {
            std::array<std::stringstream, 10> m_results {};
            #pragma omp parallel for
            for (int i { 1 }; i < 10; ++i) {
                double write_back_ratio = static_cast<double>(i) / 10;
                auto m_experiment_ptr { std::make_unique<Experiment<ImprovedSongPipeline<256, 32769, 8>, 32769>>() };
                auto& m_experiment { *m_experiment_ptr };
                //std::cout << "lambda = " << lambda << " ; write_back_ratio = " << write_back_ratio << std::endl;
                m_results[i] << " lambda = " << lambda << " ; write_back_ratio = " << write_back_ratio << std::endl;
                m_experiment.set_lambda(lambda);
                m_experiment.initialize_write_back_generator({ { 0, write_back_ratio } });
                m_experiment.reset();
                m_experiment.run_until(packet_number);
                m_experiment.report(m_results[i]);
            }
            for (int i{ 1 }; i < 10; ++i) {
                os << m_results[i].str();
            }
        }
    }
};