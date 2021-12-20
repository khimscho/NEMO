/*! \file simulate_data.cpp
 * \brief Command line user interface for the NMEA data simulator.
 *
 * This provides a very simple interface to the simulator for NMEA data so that files of a given
 * size can be readily generated for testing data loggers and transfer software.
 *
 */
/// Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
/// Hydrographic Center, University of New Hampshire.
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights to use,
/// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
/// and to permit persons to whom the Software is furnished to do so, subject to
/// the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
/// OR OTHER DEALINGS IN THE SOFTWARE.

#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <ctime>
#include "Simulator.h"

/// Report the syntax of the programme for the user.  Since the code is designed to be very
/// simple, there is only basic processing (rather than something like Boost.program_options).

void syntax(void)
{
    std::cout << "syntax: simulate_data -f <filename> -d <duration> [-s][-b]\n";
}

/// Check the command line options are appropriate, and pick out the configuration options
/// as required.
///
/// \param argc         Count of the number of arguments on the command line
/// \param argv         Vector of the arguments making up the command line
/// \param filename     (Out) Reference for the space to store the simulated data output filename
/// \param duration     (Out) Reference for the space to store the duration (seconds) of the simulated data
/// \param emit_serial  (Out) Reference for the flag: true => write NMEA0183 simulated strings
/// \param emit_binary  (Out) Reference for the flag: true => write NMEA2000 simulated data packets
/// \return True if the parse worked, otherwise false.

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
