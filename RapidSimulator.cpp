import rapid.experimental;
import rapid.experimental.bench;
import rapid.ReadWritePeer;
import rapid.SongPipeline;
import rapid.ImprovedSongPipeline;
import std;

constexpr const bool TEST_MODE { true };

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
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1)>(packet_number);
    // song pipeline does not need to test with different explicit clock max
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 2>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 3>(packet_number);
    //run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 4>(packet_number);
}

int main()
{
    if constexpr (TEST_MODE) {
        //Experiment<SinglePeer<8, 0, 1, 2>, 2> experiment;
        //Experiment<SongPipeline<128, 32769, 4>, 32769> experiment;
        
        // experiment.set_lambda(0.9);
        
        //experiment.initialize_write_back_generator({ { 0, 0.9 }, { 1, 0.1 } });
        std::ofstream os("result.txt");
        for (int j = 9; j >= 1; j--) {
            std::array<std::stringstream, 10> m_results {};
            for (int i = 9; i >= 1; i--) {
                //std::cout << "arr_period " << j << " ; wb_gap " << i << std::endl;
                Experiment<ImprovedSongPipeline<256, 32769, 4, 8192>, 32769> experiment;
                m_results[i] << " arr_period = " << j << " ; wb_gap = " << i << std::endl;
                experiment.set_arr_period(j);
                experiment.initialize_write_back_generator(i);
                experiment.reset();
                experiment.run_until(100000);
               // experiment.report(std::cout);
                experiment.report(m_results[i]);
                experiment.print_info();
            }
            for (int i{ 1 }; i < 10; ++i) {
                os << m_results[i].str();
            }
        }


    } else {
        run_experiment_bench_T1<0, 2, 1, 3>(1'000'000);
    }
    return 0;
}