export module rapid.experimental.bench;

import rapid.SongPipeline;
import rapid.ImprovedSongPipeline;
import rapid.experimental;
import std;

export template <size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t EXPLICIT_CLOCK_MAX>
class ExperimentBench {

    size_t front_buffer_size{ 0 };

public:
    ExperimentBench() = default;

    void initialize() {
        //Experiment<ImprovedSongPipeline<256, 32769, 4>, 32769> m_experiment;
    }

    void run_all(std::ostream& os, int packet_number) {
        for (double lambda{ 0.1 }; lambda <= 0.9 + 1e-8; lambda += 0.1) {
        //for (size_t arr_period{ 10 }; arr_period >= 2; arr_period--){
            std::array<std::stringstream, 10> m_results {};
            
            //#pragma omp parallel for
            for (int wb_gap { 9 }; wb_gap >= 0; wb_gap--) {
            //for (int i{ 1 }; i < 10; ++i) {
                //double write_back_ratio = static_cast<double>(i) / 10;
                int i = wb_gap;
                //auto m_experiment_ptr { std::make_unique<Experiment<ImprovedSongPipeline<256, 32769, 4>, 32769>>() };
                //auto& m_experiment { *m_experiment_ptr };
                Experiment<ImprovedSongPipeline<256, 32769, 4, 8192>, 32769> m_experiment;
                std::cout << "lambda = " << lambda << " ; write_back_gap = " << wb_gap << std::endl;
                //m_results[i] << " lambda = " << lambda << " ; write_back_ratio = " << write_back_ratio << std::endl;
                m_experiment.initialize_zipf();
                m_results[i] << " lambda = " << lambda << " ; wb_gap = " << wb_gap << std::endl;
                //m_experiment.set_arr_period(arr_period);
                m_experiment.set_lambda(lambda);
                //m_experiment.initialize_write_back_generator({ { 0, write_back_ratio } });
                m_experiment.initialize_write_back_generator(wb_gap);
                m_experiment.reset();
                m_experiment.run_until(packet_number);
                m_experiment.report(m_results[i]);
                m_experiment.print_info();
            }
            for (int i{ 0 }; i < 10; ++i) {
                os << m_results[i].str();
            }
        }
    }
};