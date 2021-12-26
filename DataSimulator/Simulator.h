/*!\file Simulator.h
 * \brief This module generates a very simple NMEA simulator
 *
 * This code models the internal state of a NMEA-generating system, keeping track of position,
 * depth, and time so that it can generate output messages at the appropriate rate.  As a small
 * efficiency improvement, the code steps the time on each call to the next event, rather than
 * doing a direct time simulation of the system.
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

#ifndef __NMEA_SIMULATOR_H__
#define __NMEA_SIMULATOR_H__

#include <string>
#include <stdint.h>
#include <memory>
#include "Writer.h"

namespace nmea {
namespace simulator {

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
    void Update(uint16_t date, double timestamp, unsigned long counter);

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
        TimeDatum(void) : datestamp(0), timestamp(-1.0), m_elapsed(0) {}
      
        uint16_t  datestamp; ///< Date in days since 1970-01-01
        double    timestamp; ///< Time in seconds since midnight

        /// \brief Test whether timestamp is valid
        bool IsValid(void) const { return timestamp >= 0.0; }
        
        /// \brief Serialise the datum into a given target
        void Serialise(Serialisable& target) const;
        
        /// \brief Give the size of the object once serialised
        uint32_t SerialisationSize(void) const
        {
            return sizeof(uint16_t)+sizeof(double) + sizeof(uint32_t);
        }
        
        /// \brief Provide something that's a printable version
        std::string printable(void) const;
        
        /// \brief Provide the raw observation (rarely required)
        uint32_t RawElapsed(void) const { return (uint32_t)m_elapsed; }
        
    private:
        friend class Timestamp;
        unsigned long m_elapsed; ///< The millisecond counter at observation time
    };

    /// \brief Generate a timestamp for the current time, if possible.
    TimeDatum Now(void);
    
    /// \brief Generate a datum for the reference information
    TimeDatum Datum(void);
    
    /// \brief Generate a string-printable representation of the information
    std::string printable(void) const;
    
    /// \brief Convert from internal count to milliseconds
    static double CountToMilliseconds(unsigned long count);

private:
    uint16_t      m_lastDatumDate;      ///< Days since 1970-01-01 at last known datum
    double        m_lastDatumTime;      ///< Time in seconds since midnight at last known datum
    unsigned long m_elapsedTimeAtDatum; ///< Internal clock elapsed time at last known datum
};

/// \class ComponentDateTime
/// \brief POD structure to maintain the broken-out components associated with a date-time
///
/// The simulator has to be able to record a current time of the system state, which is modelled
/// as a number of ticks of the system clock since the Unix epoch.  The class also allows for this
/// information to be converted into a number of different formats as required by the NMEA0183 and
/// NMEA2000 packets.

class ComponentDateTime {
public:
    unsigned long tick_count;   ///< System clock ticks for the current time
    int year;                   ///< Converted Gregorian year
    int day_of_year;            ///< Day of the current time within the year
    int hour;                   ///< Hour of the current time with the day-of-year
    int minute;                 ///< Minute of the current time within the hour
    double second;              ///< Second (with fractions) of the current time within the minute
    
    /// \brief Empty constructor for an initialised (but invalid) object
    ComponentDateTime(void);
    
    /// \brief Set the current time (in clock ticks)
    void Update(unsigned long tick_count);
    
    /// \brief Compute and return the number of days since Unix epoch for the current time 
    uint16_t DaysSinceEpoch(void) const;
    /// \brief Compute and return the total number of seconds for the current time since midnight
    double SecondsInDay(void) const;
    
    /// \brief Convert the current time into a TimeDatum for use in data construction
    Timestamp::TimeDatum Time(void) const;
};

/// \class State
/// \brief Provide the core state for the current simulator
class State {
public:
    ComponentDateTime sim_time; ///< Current timestamp for the simulation
    
    double current_depth;           ///< Depth in metres
    double measurement_uncertainty; ///< Depth sounder measurement uncertainty, std. dev.

    ComponentDateTime ref_time; ///< Reference timestamp for the ZDA/SystemTime

    double current_longitude;   ///< Longitude in degrees
    double current_latitude;    ///< Latitude in degress

private:
    friend class Engine;
    /// \brief Private constructor to that \a State objects can only be constructed by the \a Engine friend class
    State(void);

    unsigned long target_reference_time;    ///< Next clock tick count at which to update reference time state
    unsigned long target_depth_time;        ///< Next clock tick count at which to update depth state
    unsigned long target_position_time;     ///< Next clock tick count at which to update position state

    double depth_random_walk;       ///< Standard deviation, metres change in one second

    double position_step;           ///< Change in latitude/longitude per second
    double latitude_scale;          ///< Current direction of latitude change
    double last_latitude_reversal;  ///< Last time the latitude changed direction
};

/// \class Generator
/// \brief Encapsulate the requirements for generating NMEA data logged in binary format
///
/// This provides a reference mechanism for generate NMEA messages from the simulator, and writing
/// them into the output data file in the appropriate format.  This includes NMEA2000 and NMEA0183 data
/// messages for the same position, time, and depths, so that the output data file is consistent with either
/// data source.

class Generator {
public:
    /// \brief Default constructor, given the NMEA2000 object that's doing the data capture
    Generator(bool nmea0183 = true, bool nmea2000 = true);
    
    /// \brief Default destructor
    ~Generator(void) {}
    
    /// \brief Generate a timestamping message for the current state of the simulator
    void EmitTime(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    
    /// \brief Generate messages for the current configuration of the simulator
    void EmitPosition(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    
    /// \brief Generate a depth message from the current state of the simulator
    void EmitDepth(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    
    /// \brief Set verbose logging state
    void SetVerbose(bool verb) { m_verbose = verb; }
    
private:
    bool        m_verbose;  ///< Flag for verbose debug output
    Timestamp   m_now;      ///< Timestamp information for the current output state
    bool        m_serial;   ///< Emit serial strings for NMEA0183
    bool        m_binary;   ///< Emit binary data for NMEA2000
    ///
    /// \brief Generate NMEA2000 timestamp information
    void GenerateSystemTime(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    /// \brief Generate NMEA2000 positioning information
    void GenerateGNSS(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    /// \brief Generate NMEA2000 depth information
    void GenerateDepth(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    /// \brief Generate NMEA0183 timestamp (ZDA) information
    void GenerateZDA(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    /// \brief Generate NMEA0183 positioning (GPGGA) information
    void GenerateGGA(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    /// \brief Generate NMEA0183 depth (SDDBT) information
    void GenerateDBT(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output);
    
    /// \brief Compute a NMEA0183 sentence checksum
    int compute_checksum(const char *msg);
    /// \brief Convert an angle in decimal degrees into integer degrees and decimal minutes, with hemispehere indicator
    void format_angle(double angle, int& d, double& m, int& hemi);
    /// \brief Convert the current time into broken out form
    void ToDayMonth(int year, int year_day, int& month, int& day);
};

/// \class Engine
/// \brief Run the core simulator for NMEA data output
class Engine {
public:
    /// \brief Default constructor for a simulation engine
    Engine(std::shared_ptr<Generator> generator);
    ~Engine(void) {}
    
    /// \brief Move the state of the engine on to the next simulation time
    unsigned long StepEngine(std::shared_ptr<nmea::logger::Writer> output);
    
private:
    std::shared_ptr<State>      m_state;        ///< Shared pointer for the current state information
    std::shared_ptr<Generator>  m_generator;    ///< Shared pointer for the converter for state to output representation
    
    /// \brief Step the position state to a given simulation time (if required)
    bool StepPosition(unsigned long now);
    /// \brief Step the depth state to a given simulation time (if required)
    bool StepDepth(unsigned long now);
    /// \brief Step the real-world time state to a given simulation time (if required)
    bool StepTime(unsigned long now);
};

}
}

#endif
