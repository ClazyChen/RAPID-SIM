#include <iostream>

import rapid.experimental;

//Experiment<RawPipeline<64>> experiment(0.5);
Experiment<SinglePeer<64, 0, 1, 33>, 33> experiment(0.1);
//Experiment<OverlapPeer<64, 0, 2, 1, 3, 0, 2, 4>, 2> experiment(0.1);

int main()
{
    experiment.initialize_write_back_generator({
        {0, 0.1},
        {1, 0.1}
    });
    experiment.run_until(1000);
    experiment.report(std::cout);
    return 0;
}