/*!\file LogManager.h
 * \brief Objects to manage the log files, and writing data to them
 *
 * In order to ensure that multiple different loggers can write into the same output file, this
 * code centralises all of the access for log files, making new files, writing to them, removing
 * them as required, etc.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __LOG_MANAGER_H__
#define __LOG_MANAGER_H__

#include <stdint.h>
#include <Arduino.h>
#include <SD.h>
#include "serialisation.h"

namespace logger {

const int MaxLogFiles = 1000; ///< Maximum number of log files that we will create

/// \class Manager
/// \brief Handle log file access, creation, and deletion
///
/// This encapsulates the requirements for log file creation, access, and deletion.  This includes
/// log file naming and serialisation object numbering.

class Manager {
public:
    Manager(void);
    ~Manager(void);
    
    /// \brief Start a new log file, with the next available number
    void StartNewLog(void);

    /// \brief Close the current logfile (use judiciously!)
    void CloseLogfile(void);
    
    /// \brief Remove a given log file from the SD card
    boolean RemoveLogFile(const uint32_t file_num);

    /// \brief Remove all log files currently available (use judiciously!)
    void RemoveAllLogfiles(void);

    /// \brief Count the number of log files on the system
    int CountLogFiles(int filenumbers[MaxLogFiles]);
    
    /// \brief Extract information on a single log file
    void EnumerateLogFile(int lognumber, String& filename, int& filesize);
    
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
    
    /// \brief Call-through for the console log file handle
    SDFile& Console(void) { return m_consoleLog; }

private:
    SDFile      m_consoleLog;       ///< File on which to write console information
    SDFile      m_outputLog;        ///< Current output log file on the SD card
    Serialiser  *m_serialiser;      ///< Object to handle serialisation of data
    
    /// \brief Find the next log number in sequence that doesn't already exist
    uint32_t GetNextLogNumber(void);
    /// \brief Make a filename for the given log file number
    String  MakeLogName(uint32_t lognum);
};

}

#endif
