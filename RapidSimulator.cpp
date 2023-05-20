import rapid.experimental;
import rapid.experimental.bench;
import rapid.ReadWritePeer;
import rapid.SongPipeline;
import std;

constexpr const bool TEST_MODE { false };

template <size_t RID1, size_t WID1, size_t RID2, size_t WID2, size_t EXPLICIT_CLOCK_MAX>
void run_experiment_bench(int packet_number) {
    std::cout << std::endl;
    std::cout << "===================================================" << std::endl;
    std::cout << std::format(" EXPERIMENT {} {} {} {} {}", RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX) << std::endl;
    std::ofstream ofs(std::format("result-{}-{}-{}-{}-{}.txt", RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX));
    ExperimentBench<RID1, WID1, RID2, WID2, EXPLICIT_CLOCK_MAX> bench;
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
        Experiment<SongPipeline<128, 32769, 4>, 32769> experiment;
        experiment.set_lambda(0.9);
        experiment.initialize_write_back_generator({ { 0, 0.9 }, { 1, 0.1 } });
        experiment.reset();
        experiment.run_until(1000000);
        experiment.report(std::cout);
    } else {
        run_experiment_bench_T1<0, 2, 1, 3>(10'000'000);
    }
    return 0;
}