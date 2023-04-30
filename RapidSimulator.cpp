#include <iostream>

import rapid.experimental;

Experiment<RawPipeline<64>> experiment(0.5);

int main()
{
    experiment.run_until(1000000);
    experiment.report(std::cout);
    return 0;
}