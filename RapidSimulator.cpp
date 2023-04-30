#include <iostream>

import rapid.experimental;

//Experiment<RawPipeline<64>> experiment(0.5);
Experiment<SinglePeer<2, 0, 1>> experiment(1);

int main()
{
    experiment.initialize_write_back_generator({
        {0, 1}
    });
    experiment.run_until(1000000);
    experiment.report(std::cout);
    return 0;
}