/*!\file Writer.h
 * \brief Objects to manage the log files, and writing data to them
 *
 * In order to ensure that multiple different loggers can write into the same output file, this
 * code centralises all of the access for log files, making new files, writing to them, removing
 * them as required, etc.
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

#ifndef __LOG_WRITER_H__
#define __LOG_WRITER_H__

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "serialisation.h"

namespace nmea {
namespace logger {

/// \class Writer
/// \brief Handle log file creation, generation, and access
///
/// This encapsulates the requirements for log file creation and access.

class Writer {
public:
    Writer(std::string const& filename);
    ~Writer(void);
        
    /// \enum PacketIDs
    /// \brief Symbolic definition for the packet IDs used to serialise the messages from NMEA2000
    enum PacketIDs {
        Pkt_SystemTime = 1,     ///< Real-time information from GNSS (or atomic clock)
        Pkt_Attitude = 2,       ///< Platform roll, pitch, yaw
        Pkt_Depth = 3,          ///< Observed depth, offset, and depth range
        Pkt_COG = 4,            ///< Course and speed over ground
        Pkt_GNSS = 5,           ///< Position information and metrics
        Pkt_Environment = 6,    ///< Temperature, Humidity, and Pressure
        Pkt_Temperature = 7,    ///< Temperature and source
        Pkt_Humidity = 8,       ///< Humidity and source
        Pkt_Pressure = 9,       ///< Pressure and source
        Pkt_NMEAString = 10,    ///< A generic NMEA0183 string, in raw format
        Pkt_LocalIMU = 11,      ///< Logger's on-board IMU
        Pkt_Metadata = 12,      ///< Logger identification information
        Pkt_Algorithms = 13,    ///< Algorithms and parameters to apply to the data, by preference
        Pkt_JSON = 14,          ///< JSON metadata element to pass on to cloud processing
        Pkt_NMEA0183ID = 15     ///< Acceptable NMEA0183 sentence ID for filtering
    };
    
    /// \brief Write a packet into the current log file
    void Record(PacketIDs pktID, Serialisable const& data);
    
private:
    FILE        *m_outputLog;       ///< Current output log file on the SD card
    Serialiser  *m_serialiser;      ///< Object to handle serialisation of data
};

}
}

#endif
