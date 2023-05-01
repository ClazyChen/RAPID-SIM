#include <iostream>
#include <fstream>
#include <format>

import rapid.experimental;
import rapid.experimental.bench;
import rapid.ReadWritePeer;

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
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2>(packet_number);
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 2>(packet_number);
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 3>(packet_number);
    run_experiment_bench<RID1, WID1, RID2, WID2, CLOCK_MAX(RID1, WID1) + CLOCK_MAX(RID2, WID2) / 2 * 4>(packet_number);
}

int main()
{
    run_experiment_bench_T1<0, 2, 1, 3>(1000000);
    return 0;
}