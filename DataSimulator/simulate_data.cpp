/*! \file simulate_data.cpp
 * \brief Command line user interface for the NMEA data simulator.
 *
 * This provides a very simple interface to the simulator for NMEA data so that files of a given
 * size can be readily generated for testing data loggers and transfer software.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping & NOAA-UNH
 * Joint Hydrographic Center.  All Rights Reserved.
 */

#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <ctime>
#include "Simulator.h"

void syntax(void)
{
    std::cout << "syntax: simulate_data -f <filename> -d <duration> [-s][-b]\n";
}

bool check_options(int argc, char **argv, std::string& filename, unsigned long& duration, bool& emit_serial, bool& emit_binary)
{
    int ch;
    while ((ch = getopt(argc, argv, "f:d:sb")) != -1) {
        switch (ch) {
            case 'f':
                filename = std::string(optarg);
                break;
            case 'd':
                duration = strtoul(optarg, NULL, 0) * CLOCKS_PER_SEC;
                break;
            case 's':
                emit_serial = true;
                break;
            case 'b':
                emit_binary = true;
                break;
            case '?':
            default:
                syntax();
                return false;
                break;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    std::string filename;
    unsigned long duration;
    bool emit_nmea0183, emit_nmea2000;
    if (!check_options(argc, argv, filename, duration, emit_nmea0183, emit_nmea2000))
        return 1;
    
    std::shared_ptr<nmea::simulator::Generator> gen(new nmea::simulator::Generator(emit_nmea0183, emit_nmea2000));
    std::shared_ptr<nmea::logger::Writer> writer(new nmea::logger::Writer(filename));
    nmea::simulator::Engine engine(gen);
    
    unsigned long first_time, current_time;
    
    current_time = first_time = engine.StepEngine(writer);
    std::cout << "First generation time step: " << current_time << "\n";
    while (current_time - first_time < duration) {
        current_time = engine.StepEngine(writer);
        std::cout << "Step to time: " << current_time << "\n";
    }
    
    return 0;
}
