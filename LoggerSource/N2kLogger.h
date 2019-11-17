/*
 * \file N2kLogger.h
 * \brief Main code for a NMEA2000 message handler with time, position, depth logging and timestamping
 * 
 * This code is designed to form a simple data logger for NMEA2000, which keeps track of time and stores
 * position, depth, and some environmental information into the SD card attached to the board.  The goal
 * here is to store sufficient information to make this a useful crowd-sourced bathymetry logging engine,
 * but at the cheapest possible price.
 * 
 * 2019-08-25.
 */

#ifndef __N2K_LOGGER_H__
#define __N2K_LOGGER_H__

#include <stdint.h>
#include <Arduino.h>
#include <SD.h>
#include <NMEA2000.h>
#include "serialisation.h"

/// \class Timestamp
/// \brief Generate a timestamp for an instant based on elapsed time to last known time
///
/// This class attempts to generate plausible timestamps for any given instant based on
/// the elapsed time counter (usually in milliseconds) and a known time instant.  This
/// isn't the most elegant way to do this, but it works good enough for the purpose.

class Timestamp {
public:
    /// \brief Default constructor with invalid setup
    Timestamp(void);
    
    /// \brief Provide a new observation of a known (UTC) time
    void Update(uint16_t date, double timestamp);
    /// \brief Provide a new observation of a known (UTC) time and elapsed time
    void Update(uint16_t date, double timestamp, unsigned long ms_counter);

    /// \brief Indicate whether a good timestamp has been generated yet
    inline bool IsValid(void) const { return m_lastDatumTime >= 0.0; }
    
    /// \struct TimeDatum
    /// \brief POD for a time instant
    ///
    /// This provides a data object that represents a single point in time, with real-time interpolated
    /// from the elapsed time at construction based on the real-time reference and elapsed time
    /// offset in the supporting \a Timestamp object.
    
    struct TimeDatum {
    public:
        /// \brief Constructor, generating an timestamp based on construction time
        TimeDatum(void) : datestamp(0), timestamp(-1.0), m_elapsed(millis()) {}
      
        uint16_t  datestamp; ///< Date in days since 1970-01-01
        double    timestamp; ///< Time in seconds since midnight

        /// \brief Test whether timestamp is valid
        bool IsValid(void) const { return timestamp >= 0.0; }
        
        /// \brief Serialise the datum into a given target
        void Serialise(Serialisable& target) const;
        
        /// \brief Give the size of the object once serialised
        uint32_t SerialisationSize(void) const { return sizeof(uint16_t)+sizeof(double); }
        
        /// \brief Provide something that's a printable version
        String printable(void) const;
        
        /// \brief Provide the raw observation (rarely required)
        uint32_t RawElapsed(void) const { return (uint32_t)m_elapsed; }
        
    private:
        unsigned long m_elapsed; ///< The millisecond counter at observation time
    };

    /// \brief Generate a timestamp for the current time, if possible.
    TimeDatum Now(void);
    
    /// \brief Generate a string-printable representation of the information
    String printable(void) const;

private:
    uint16_t      m_lastDatumDate;      ///< Days since 1970-01-01 at last known datum
    double        m_lastDatumTime;      ///< Time in seconds since midnight at last known datum
    unsigned long m_elapsedTimeAtDatum; ///< Internal clock elapsed time at last known datum
};

/// \class N2kLogger
/// \brief Encapsulate N2K message handler and SD logger
///
/// This provides a tNMEA2000::tMsgHandler sub-class that manages the messages required for the CSB
/// data logger, and writes the translated messages to SD card for later capture.  The code is heavily
/// based on the example N2K->0183 converter that comes as an example with the NMEA2000 library.

class N2kLogger : public tNMEA2000::tMsgHandler {
public:
    /// \brief Default constructor, given the NMEA2000 object that's doing the data capture
    N2kLogger(tNMEA2000 *source);
    
    /// \brief Default destructor
    ~N2kLogger(void);
    
    /// \brief Message handler for the tNMEA2000::tMsgHandler interface
    void HandleMsg(const tN2kMsg &N2kMsg);
    
    /// \brief Start a new log file, with the next available number
    void StartNewLog(void);

    /// \brief Close the current logfile (use judiciously!)
    void CloseLogfile(void);
    
    /// \brief Remove a given log file from the SD card
    boolean RemoveLogFile(const uint32_t lognum);

    /// \brief Remove all log files currently available (use judiciously!)
    void RemoveAllLogfiles(void);

    /// \brief Generate a software version string, as required by NMEA2000 library
    String SoftwareVersion(void) const;

    /// \brief Set verbose logging state
    void SetVerbose(bool verb) { m_verbose = verb; }
    
private:
    bool        m_verbose;          ///< Flag for verbose debug output
    SDFile      m_outputLog;        ///< Current output log file on the SD card
    SDFile      m_console;          ///< Log for monitoring of the data stream
    Timestamp   m_timeReference;    ///< Time reference information for timestamping records
    Serialiser  *m_serialiser;      ///< Object to handle serialisation of data
    
    /// \brief Find the next log number in sequence that doesn't already exist
    uint32_t GetNextLogNumber(void);
    /// \brief Make a filename for the given log file number
    String  MakeLogName(uint32_t lognum);
    
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
        Pkt_Pressure = 9        ///< Pressure and source
    };
    /// \brief Translate and serialise the real-time information from GNSS (or atomic clock)
    void HandleSystemTime(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise the platform attitude information
    void HandleAttitude(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise the observed depth information
    void HandleDepth(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise the course and speed over ground information
    void HandleCOG(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise the GNSS observation information
    void HandleGNSS(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise the temperature, humidity, and pressure information
    void HandleEnvironment(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise a temperature observation
    void HandleTemperature(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise an external temperature observation
    void HandleExtTemperature(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise a humidity observation
    void HandleHumidity(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
    /// \brief Translate and serialise a pressure observation
    void HandlePressure(Timestamp::TimeDatum const& t, tN2kMsg const& msg);
};

#endif
