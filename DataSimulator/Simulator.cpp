/*! \file Simulator.cpp
 * \brief Generate faked NMEA data
 *
 * This manages the simulation of NMEA position, time, and depth data in NMEA2000 and NMEA0183 formats,
 * as output to binary format in the style of the low-cost data logger.
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

#include <ctime>
#include <cmath>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include "Simulator.h"

namespace nmea { namespace simulator {

/// Constructor for a timestamp holder.  This initialises with the last datum time set to a
/// negative value so that it reads as invalid until something provides an update.

Timestamp::Timestamp(void)
{
    m_lastDatumTime = -1.0; // So !IsValid() == true
}

/// Update the current timestamp with a known date and time.  The date (days since 1970-01-01)
/// and time (seconds past midnight) are the standard used for NMEA2000 SystemTime messages.
/// The code picks the microcontroller's elapsed time (using millis() call) as soon as possible, and
/// uses this as the time reference code that should be consistent between all timestamps.  Of
/// course, there is no guarantee as to the latency with which this has been generated, so behaviours
/// may vary.
///
/// \param date     Days since 1970-01-01
/// \param timestamp    Seconds since midnight on the day

void Timestamp::Update(uint16_t date, double timestamp)
{
    unsigned long t = static_cast<unsigned long>(std::clock());
    m_lastDatumDate = date;
    m_lastDatumTime = timestamp;
    m_elapsedTimeAtDatum = t;
}

/// Update the current timestamp with a known date and time, and externally generated reference time.
/// This initialises (or updates) the object with a time reference point (i.e., real time associated with the
/// elapsed time, which should be monotonic), and therefore allows for timestamps to be generated for
/// other data packets subsequently.  The date and timestamp are as for NMEA2000 SystemTime packets.
/// This form of the code allows the user to pick up a low-latency counter for the elapsed time (typically in
/// milliseconds since boot) when a time reference becomes available, and then use it to generate an
/// update in a more leisurely fashion.
///
/// \param date     Days since 1970-01-01
/// \param timestamp    Seconds since midnight on the day
/// \param ms_counter   Reference elapsed time count associated with the real-time update

void Timestamp::Update(uint16_t date, double timestamp, unsigned long ms_counter)
{
    m_lastDatumDate = date;
    m_lastDatumTime = timestamp;
    m_elapsedTimeAtDatum = ms_counter;
}

/// Generate a timestamp for the current instant, based on the reference time memorised in
/// the Timestamp object.  This constructs an elapsed time as soon as possible, and then corrects
/// if required if the elapsed time has rolled over since the datum, or if there has been more than a
/// day since the datum was established (this should be very rare).  The returned TimeDatum maintains
/// the original raw elapsed time instant, so that post-processed estimates are also possible.  Note that
/// the accuracy of the real time estimate depends strongly on the accuracy of the internal time
/// reference, which can be very dubious.  A non-causal solution might be a lot better.
///
/// \return TimeDatum with an estimate of the current real time

Timestamp::TimeDatum Timestamp::Now(void)
{
    TimeDatum rtn;

    unsigned long diff;
    if (rtn.RawElapsed() < m_elapsedTimeAtDatum) {
        // We wrapped the counter
        diff = rtn.RawElapsed() + (0xFFFFFFFFUL - m_elapsedTimeAtDatum) + 1;
    } else {
        diff = rtn.RawElapsed() - m_elapsedTimeAtDatum + 1;
    }
    double time_now = m_lastDatumTime + diff/CLOCKS_PER_SEC;
    
    rtn.datestamp = m_lastDatumDate;
    if (time_now > 24.0*60.0*60.0) {
        // We've skipped a day
        ++rtn.datestamp;
        time_now -= 24.0*60.0*60.0;
    }
    rtn.timestamp = time_now;
    return rtn;
}

/// Generate a timestamp for the current time of the simulation
///
/// \return Timestamp::TimeDatum for the current time of the siulator

Timestamp::TimeDatum Timestamp::Datum(void)
{
    TimeDatum rtn;
    
    rtn.m_elapsed = m_elapsedTimeAtDatum;
    rtn.datestamp = m_lastDatumDate;
    rtn.timestamp = m_lastDatumTime;
    return rtn;
}

/// Generate a printable version of the reference real-time in the object.
///
/// \return String with a human-readable (non-compact) version of the reference time.

std::string Timestamp::printable(void) const
{
    std::ostringstream os;
    os << "R: " << m_lastDatumDate << " days, " << m_lastDatumTime << "s, at counter "
    << m_elapsedTimeAtDatum << " clocks since boot";
    return std::string(os.str());
}

/// Generate a binary representation of the real time estimate in the TimeDatum for
/// serialisation.  This stores the estimate date (in days) and time (in seconds) into
/// the supplied before, so that it can be serialised later.
///
/// \param s    Reference for the buffer into which to add the TimeDatum information.

void Timestamp::TimeDatum::Serialise(Serialisable& s) const
{
    s += datestamp;
    s += timestamp;
    s += static_cast<uint32_t>(Timestamp::CountToMilliseconds(RawElapsed()));
    std::cout << "debug: serialising TimeDatum for " << datestamp << " days, " << timestamp << " seconds, elapsed ticks " << RawElapsed() << "\n";
}

/// Generate a printable version of the estimated real time in the TimeDatum.
///
/// \return String with the human-readable (non-compact) version of the real time estimate.

std::string Timestamp::TimeDatum::printable(void) const
{
    std::ostringstream os;
    os << "T: " << datestamp << " days, " << timestamp << " s";
    return std::string(os.str());
}

/// Convert from the internal clock-tick count to milliseconds.  The simululator can run the
/// code at faster than millisecond rates; this converts by CLOCKS_PER_SEC to make something
/// that's human readable.
///
/// \param count    Internal count to convert
/// \return Number of milliseconds reflected in the input count

double Timestamp::CountToMilliseconds(unsigned long count)
{
    const double conversion_factor = 1000.0 / CLOCKS_PER_SEC;
    return count * conversion_factor;
}

/// Default constructor for a ComponentDateTime (for the start of 2020-01-01/00:00:00)

ComponentDateTime::ComponentDateTime(void)
{
    tick_count = 0;
    year = 2020;
    day_of_year = 0;
    hour = 0;
    minute = 0;
    second = 0.0;
}

/// Increment the data/time by a simulation count.  This rolls over the difference between
/// the new count given and the current tick count to update the stored date/time.  This
/// means that there may be problems if the simulated time goes backwards for any reason.
///
/// \param new_count    Simulation count at which the stored time should be updated.

void ComponentDateTime::Update(unsigned long new_count)
{
    unsigned long delta = new_count - tick_count;
    second += static_cast<double>(delta)/CLOCKS_PER_SEC;
    if (second >= 60.0) {
        minute++;
        second -= 60.0;
        if (minute >= 60) {
            hour++;
            minute = 0;
            if (hour >= 24) {
                hour = 0;
                day_of_year++;
                if (day_of_year >= 365) {
                    // This, of course, is not accurate ... but it is simple.
                    day_of_year = 0;
                    year++;
                }
            }
        }
    }
    tick_count = new_count;
}

/// Compute the number of days that have elapsed since the timestamp epoch (same as
/// Unix epoch).  This is approximate, since it assumes 365.25 days/year, but should work
/// for short simulation times.
///
/// \return Days since epoch for the stored time
uint16_t ComponentDateTime::DaysSinceEpoch(void) const
{
    return day_of_year + static_cast<uint16_t>((year - 1970)*365.25);
}

/// Compute the number of seconds into the current day for the stored time.
///
/// \return Seconds since midmight in the current day for the stored time

double ComponentDateTime::SecondsInDay(void) const
{
    return second + minute * 60.0 + hour * 3600.0;
}

/// Convert the stored time to a Timestamp::TimeDatum for output.
///
/// \return Current time converted to a Timestamp::TimeDatum

Timestamp::TimeDatum ComponentDateTime::Time(void) const
{
    Timestamp t;
    t.Update(DaysSinceEpoch(), SecondsInDay(), tick_count);
    return t.Datum();
}

bool iset = false;  ///< Global for Gaussian variate generator (from Numerical Recipies)
float gset;         ///< Global state for Gaussian variate generator (from Numerical Recipies)

/// Helper function to generate a unit uniform RV from the underlying library's
/// default random() function.  The statistical qualities of these RVs therefore rely
/// on the quality of the underlying support library, which can vary widely.  Since this is
/// really just to generate some data that's useful for testing processing, the statistical
/// quality probably isn't that important.
///
/// \return Uniform RV in the range [0,1)

double unit_uniform(void)
{
    const double maximum_random = (1UL<<31) - 1;
    double n = random()/maximum_random;
    return n;
}

/// Simulate a unit Gaussian variate, using the method of Numerical Recipes.  Note
/// that this preserves state in global variables, so it isn't great, but it does do
/// the job, and gets most benefit from the random simulation calls.  The code does,
/// however, rely on the quality of the standard random() call in the supporting
/// environment, and therefore the statistical quality of the variates is only as good
/// as the underlying linear PRNG.
///
/// \return A sample from a zero mean, unit standard deviation, Gaussian distribution.

double unit_normal(void)
{
    float fac, rsq, v1, v2;
    float u, v, r;

    if (!iset) {
        do {
            u = unit_uniform();
            v = unit_uniform();
            v1 = 2.0*u - 1.0;
            v2 = 2.0*v - 1.0;
            rsq = v1*v1 + v2*v2;
        } while (rsq >= 1.0 || rsq == 0.0);
        fac = sqrt(-2.0*log(rsq)/rsq);
        gset = v1*fac;
        iset = true;
        r = v2*fac;
    } else {
        iset = false;
        r = gset;
    }
    return r;
}

/// Default constructor for simulator state, setting up the initial conditions for the simulator
/// (43N, 75W, depth 10m) and the parameters for the depth random walk and position updates.

State::State(void)
{
    current_depth = 10.0;
    
    current_longitude = -75.0;
    current_latitude = 43.0;
    
    target_reference_time = target_depth_time = target_position_time = 0;
    measurement_uncertainty = 0.06;
    depth_random_walk = 0.02;
    
    position_step = 3.2708e-06;
    latitude_scale = +1.0;
    last_latitude_reversal = 0.0;
}

/// Default constructor for the simulation data generator.  The generator can be used to generate
/// NMEA0183 or NMEA2000 data packets based on the current state, or both.  It does, however, have to
/// generate at least one of the two, and will default to NMEA2000 if neither is requested on instantiation.
///
/// \param emit_nmea0183    Flag: true => emit NMEA0183 ASCII sentences for the simulator state
/// \param emit_nmea2000    Flag: true => emit NMEA2000 binary sentences for the simulator state

Generator::Generator(bool emit_nmea0183, bool emit_nmea2000)
: m_verbose(false), m_serial(emit_nmea0183), m_binary(emit_nmea2000)
{
    m_now.Update(0, 0.0, 0UL);
    if (!emit_nmea0183 && !emit_nmea2000) {
        // Have to emit something!
        std::cerr << "warning: user asked for neither NMEA0183 or NMEA2000; defaulting to generating NMEA2000\n";
        m_binary = true;
    }
}

/// Generate checksums for NMEA0183 messages.  The checksum is a simple XOR of all of the
/// contents of the message between (and including) the leading "$" and trailing "*", and
/// is returned as an 8-bit integer.  This needs to be converted to two hex digits before
/// appending to the end of the message to complete it.  The code here does not modify the
/// message, however.
///
/// \param msg  NMEA0183-formatted message for which to compute the byte-wise XOR checksum
/// \return XOR of the contents of \a msg, taken an 8-bit byte at a time

int Generator::compute_checksum(const char *msg)
{
    int chk = 0;
    int len = static_cast<int>(strlen(msg));
    for (int i = 1; i < len-1; ++i) {
        chk ^= msg[i];
    }
    return chk;
}

/// Break an angle in degrees into the components required to format for ouptut
/// in a NMEA0183 GGA message (i.e., integer degrees and decimal minutes, with an
/// indicator for which hemisphere to use).  To allow this to be uniform, the code
/// assumes a positive angle is hemisphere 1, and negative angle hemisphere 0.  It's
/// up to the caller to decide what decorations are used for those in the display.
///
/// \param angle    Angle (degrees) to break apart
/// \param d        (Out) Reference to space for the integer part of the angle (degrees)
/// \param m        (Out) Reference to space for the decimal minutes part of the angle
/// \param hemi     (Out) Reference to space for a hemisphere indicator

void Generator::format_angle(double angle, int& d, double& m, int& hemi)
{
    if (angle > 0.0) {
        hemi = 1;
    } else {
        hemi = 0;
        angle = -angle;
    }

    d = (int)angle;
    m = angle - d;
}

/// Generate a simulated position (GGA) message.  This formats the current state for position as
/// required for GGA messages, and then appends a standard trailer to meet the NMEA0183 requirements.
/// Since this trailer is always fixed, it won't exercise potential modifications based on changes
/// to the ellipsoid separation or antenna height, etc., although those are expected to be very low
/// priority for the expected use of the data being generated.
///
/// \param state    Simulator state to use for generation
/// \param output   Output writer to use for serialisation of the simulated position report

void Generator::GenerateGGA(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    char msg[255];
    int pos = sprintf(msg, "$GPGGA,%02d%02d%06.3f,",
                            state->sim_time.hour, state->sim_time.minute, state->sim_time.second);
    int degrees;
    double minutes;
    int hemisphere;
    format_angle(state->current_latitude, degrees, minutes, hemisphere);
    pos += sprintf(msg + pos, "%02d%09.6lf,%c,", degrees, minutes, hemisphere == 1 ? 'N' : 'S');
    format_angle(state->current_longitude, degrees, minutes, hemisphere);
    pos += sprintf(msg + pos, "%03d%09.6lf,%c,", degrees, minutes, hemisphere == 1 ? 'E' : 'W');
    pos += sprintf(msg + pos, "3,12,1.0,-19.5,M,22.5,M,0.0,0000*");
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);
    
    Serialisable data(255);
    data += static_cast<uint32_t>(Timestamp::CountToMilliseconds(state->sim_time.tick_count));
    data += msg;
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_NMEAString, data);
}

/// Generate a simulated depth (DBT) NMEA0183 message.  The depth simulation is conducted in metres, and then
/// converted to feet and fathoms to fill out the overall message.
///
/// \param state    Simulator state to use for generation
/// \param output   Output writer to use for serialisation of the simulated position report

void Generator::GenerateDBT(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    double depth_metres = state->current_depth + state->measurement_uncertainty*unit_normal();
    double depth_feet = depth_metres * 3.2808;
    double depth_fathoms = depth_metres * 0.5468;

    char msg[255];
    int pos = sprintf(msg, "$SDDBT,%.1lf,f,%.1lf,M,%.1lf,F*", depth_feet, depth_metres, depth_fathoms);
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);
    
    Serialisable data(255);
    data += static_cast<uint32_t>(Timestamp::CountToMilliseconds(state->sim_time.tick_count));
    data += msg;
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_NMEAString, data);
}

/// Convert from a year, and day-of-year (a.k.a., albeit inaccurately, Julian Day) to
/// a month/day pair, as required for ouptut of ZDA information.  Keeping the time
/// as a day of the year makes the simulation simpler ...
///
/// \param year         Current year of the simulation
/// \param year_day     Current day-of-year of the simulation
/// \param month        (Out) Reference for store of the month of the year
/// \param day          (Out) Reference for store of the day of the month

void Generator::ToDayMonth(int year, int year_day, int& month, int& day)
{
    unsigned     leap;
    static unsigned months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    
    /* Determine whether this is a leap year.  Leap years in the Gregorian
     * calendar are years divisible by four, unless they are also a century
     * year, except when the century is also divisible by 400.  Thus 1900 is
     * not a leap year (although it is divisible by 4), but 2000 is, because
     * it is divisible by 4 and 400).
     */
    if ((year%4) == 0) {
            /* Potentially a leap year, check for century year */
            if ((year%100) == 0) {
                    /* Century year, check for 400 years */
                    if ((year%400) == 0)
                            leap = 1;
                    else
                            leap = 0;
            } else
                    leap = 1;
    } else
            leap = 0;
    day = year_day + 1; // External is [0, 364], but we assume here [1, 365]
    
    months[1] += leap;      /* Correct February */
    
    /* Compute month by reducing days until we have less than the next months
     * total number of days.
     */
    month = 0;
    while (day > months[month]) {
            day -= months[month];
            ++month;
    }
    ++month; // External is [1, 12] but here it's [0, 11]
    
    months[1] -= leap;      /* Uncorrect February */
}

/// Generate a simulated timestamp (ZDA) message using the current state information.
///
/// \param state    Simulator state to use for generation
/// \param output   Output writer to use for serialisation of the simulated position report

void Generator::GenerateZDA(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    char msg[255];
    int day, month;
    
    // We track day-of-year (a.k.a. Julian Day), so we need to convert to day/month for output
    ToDayMonth(state->sim_time.year, state->sim_time.day_of_year, month, day);
    int pos = sprintf(msg, "$GPZDA,%02d%02d%06.3lf,%02d,%02d,%04d,00,00*",
                      state->sim_time.hour, state->sim_time.minute, state->sim_time.second,
                      day, month, state->sim_time.year);
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);
    
    Serialisable data(255);
    data += static_cast<uint32_t>(Timestamp::CountToMilliseconds(state->sim_time.tick_count));
    data += msg;
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_NMEAString, data);
}

void Generator::GenerateSystemTime(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    Serialisable s(sizeof(uint16_t) + sizeof(double) + sizeof(unsigned long) + 1);
    s += state->ref_time.DaysSinceEpoch();
    s += state->ref_time.SecondsInDay();
    s += static_cast<uint32_t>(Timestamp::CountToMilliseconds(state->ref_time.tick_count));
    s += static_cast<uint8_t>(0);
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_SystemTime, s);
}

void Generator::GenerateGNSS(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    Timestamp tm;
    tm.Update(state->sim_time.DaysSinceEpoch(), state->sim_time.SecondsInDay(), state->sim_time.tick_count);
    
    Serialisable data(tm.Now().SerialisationSize() + 2*sizeof(uint16_t) + 8*sizeof(double) + 5);
    uint8_t     rx_type = 0, // GPS
                rx_method = 2, // DGNSS
                num_SVs = 12,
                nRefStations = 1,
                refStationType = 4; // All constellations
    uint16_t    refStationID = 12312;
    double      altitude = -19.323,
                hdop = 1.5,
                pdop = 2.2,
                sep = 22.3453,
                correctionAge = 2.32;
    
    tm.Datum().Serialise(data);
    data += state->sim_time.DaysSinceEpoch();
    data += state->sim_time.SecondsInDay();
    data += state->current_latitude;
    data += state->current_longitude;
    data += altitude;
    data += rx_type;
    data += rx_method;
    data += num_SVs;
    data += hdop;
    data += pdop;
    data += sep;
    data += nRefStations;
    data += refStationType;
    data += refStationID;
    data += correctionAge;
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_GNSS, data);
}

void Generator::GenerateDepth(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    Timestamp tm;
    tm.Update(state->sim_time.DaysSinceEpoch(), state->sim_time.SecondsInDay(), state->sim_time.tick_count);
    
    Serialisable data(tm.Now().SerialisationSize() + 3*sizeof(double));
    const double offset = 0.0, range = 200.0;
    
    tm.Datum().Serialise(data);
    data += state->current_depth;
    data += offset;
    data += range;
    output->Record(nmea::logger::Writer::PacketIDs::Pkt_Depth, data);
}

void Generator::EmitTime(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    if (m_binary) GenerateSystemTime(state, output);
    if (m_serial) GenerateZDA(state, output);
}

void Generator::EmitPosition(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    if (m_binary) GenerateGNSS(state, output);
    if (m_serial) GenerateGGA(state, output);
}

void Generator::EmitDepth(std::shared_ptr<State> state, std::shared_ptr<nmea::logger::Writer> output)
{
    if (m_binary) GenerateDepth(state, output);
    if (m_serial) GenerateDBT(state, output);
}

Engine::Engine(std::shared_ptr<Generator> generator)
: m_generator(generator)
{
    m_state = std::shared_ptr<State>(new State());
}

bool Engine::StepDepth(unsigned long now)
{
    if (now < m_state->target_depth_time) return false;
    
    m_state->current_depth += m_state->depth_random_walk*unit_normal();
    
    m_state->target_depth_time = now + CLOCKS_PER_SEC +
                                 static_cast<int>(CLOCKS_PER_SEC*unit_uniform());
    return true;
}

bool Engine::StepPosition(unsigned long now)
{
    if (now < m_state->target_position_time) return false;
    
    m_state->current_latitude += m_state->latitude_scale * m_state->position_step;
    m_state->current_longitude += 1.0 * m_state->position_step;

    if ((now - m_state->last_latitude_reversal)/3600.0 > CLOCKS_PER_SEC) {
        m_state->latitude_scale = -m_state->latitude_scale;
        m_state->last_latitude_reversal = now;
    }

    m_state->target_position_time = now + CLOCKS_PER_SEC;
    return true;
}

bool Engine::StepTime(unsigned long now)
{
    if (now < m_state->target_reference_time) return false;
    
    m_state->ref_time.Update(now);
    
    m_state->target_reference_time = m_state->ref_time.tick_count + CLOCKS_PER_SEC;
    
    return true;
}

unsigned long Engine::StepEngine(std::shared_ptr<nmea::logger::Writer> output)
{
    unsigned long next_time = std::min(m_state->target_depth_time, m_state->target_position_time);
    next_time = std::min(next_time, m_state->target_reference_time);
    
    m_state->sim_time.Update(next_time);
    
    bool time_change = StepTime(next_time);
    bool position_change = StepPosition(next_time);
    bool depth_change = StepDepth(next_time);
    
    if (time_change) {
        m_generator->EmitTime(m_state, output);
    }
    if (position_change) {
        m_generator->EmitPosition(m_state, output);
    }
    if (depth_change) {
        m_generator->EmitDepth(m_state, output);
    }
    
    return next_time;
}

}
}
