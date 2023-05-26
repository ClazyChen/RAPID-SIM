import rapid.experimental;
import rapid.experimental.bench;
import rapid.ReadWritePeer;
import rapid.SongPipeline;
import rapid.ImprovedSongPipeline;
import std;
#include "stdlib.h"
constexpr const bool TEST_MODE { false };

template <size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t EXPLICIT_CLOCK_MAX>
void run_experiment_bench(int packet_number) {
    std::cout << std::endl;
    std::cout << "===================================================" << std::endl;
    std::cout << std::format(" EXPERIMENT {} {} {} {} {}", RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX) << std::endl;
    std::ofstream ofs(std::format("result-{}-{}-{}-{}-{}.txt", RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX));
    ExperimentBench<RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX> bench;
    bench.initialize();
    bench.run_all(ofs, packet_number);
}

template <size_t RID1, size_t WID1, size_t RID2, size_t WID2>
void run_experiment_bench_T1(int packet_number)
{
    std::cout << "1" << std::endl;
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1)>(packet_number);
    // song pipeline does not need to test with different explicit clock max
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 2>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 3>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 4>(packet_number);
}

template <size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t EXPLICIT_CLOCK_MAX>
void run_song_experiment_bench(int packet_num, size_t fc_num, size_t fixed_flow_num, size_t pkt_gap, size_t burst_gap, size_t burst_ratio, size_t front_buffer_size, size_t pir_type) {
    std::cout << std::endl;
    std::cout << "===================================================" << std::endl;
    std::cout << std::format(" EXPERIMENT {} {} {} {} {} {} {}", fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_ratio, front_buffer_size, pir_type) << std::endl;
    std::ofstream ofs(std::format("result_{}_{}_{}_{}_{}_{}_{}.txt", fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_ratio, front_buffer_size, pir_type));
    ExperimentBench<RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX> bench;
    bench.initialize(fc_num, fixed_flow_num, pkt_gap, burst_gap, burst_ratio, front_buffer_size, pir_type);
    bench.run_all(ofs, packet_num);
}

int main(int argc, char const* argv[])
{
    if constexpr (TEST_MODE) {
        //Experiment<SinglePeer<8, 0, 1, 2>, 2> experiment;
        //Experiment<SongPipeline<128, 32769, 4>, 32769> experiment;
        
        // experiment.set_lambda(0.9);
        
        //experiment.initialize_write_back_generator({ { 0, 0.9 }, { 1, 0.1 } });
        std::ofstream os;
        os.open("result.txt", std::ios_base::out);
        //for (int j = 9; j >= 1; j--) {
        for (double lambda{ 0.1 }; lambda <= 0.9 + 1e-8; lambda += 0.1) {
            std::array<std::stringstream, 10> m_results {};
            for (int i = 9; i >= 0; i--) {
                std::cout << "lambda " << lambda << " ; wb_gap " << i << std::endl;
                Experiment<ImprovedSongPipeline<256, 32769, 4, 8192>, 32769> experiment;
                //m_results[i] << " arr_period = " << j << " ; wb_gap = " << i << std::endl;
                m_results[i] << " lambda = " << lambda << " ; wb_gap = " << i << std::endl;
                //experiment.set_arr_period(j);
                experiment.set_lambda(lambda);
                experiment.initialize_write_back_generator(i);
                experiment.reset();
                experiment.run_until(100000);
                //experiment.report(std::cout);
                experiment.report(m_results[i]);
                experiment.print_info();
                os << m_results[i].str();
            }
        }
        os.close();


    } else {
        run_experiment_bench_T1<0, 2, 1, 3>(100'000);
    }
    return 0;
}