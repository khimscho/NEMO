#include <stdlib.h>
#include <stdint.h>
#include <limits>
#include <iostream>

#include "Simulator.h"


const size_t MAX_ITER = 1000000;


int main(int argc, char **argv)
{
    double test_unit_normal[MAX_ITER];

    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    double mean = 0.0;
    for (size_t i = 0; i < MAX_ITER; i++) {
        test_unit_normal[i] = nmea::simulator::unit_normal();
        if (test_unit_normal[i] > max) {
            max = test_unit_normal[i];
        }
        if (test_unit_normal[i] < min) {
            min = test_unit_normal[i];
        }
        mean += test_unit_normal[i];
    }
    mean /= MAX_ITER;

    std::cout << "Num iterations: " << MAX_ITER << "\n";
    std::cout << "Mean: " << mean << ", min: " << min << ", max: " << max << "\n";

    return 0;
}
