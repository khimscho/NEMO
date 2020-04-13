/*!\file Writer.h
 * \brief Objects to manage the log files, and writing data to them
 *
 * In order to ensure that multiple different loggers can write into the same output file, this
 * code centralises all of the access for log files, making new files, writing to them, removing
 * them as required, etc.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 */

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
        Pkt_NMEAString = 10     ///< A generic NMEA0183 string, in raw format
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
