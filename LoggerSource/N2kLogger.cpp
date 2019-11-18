/*!\file N2kLogger.cpp
 * \brief Implementation of a simple NMEA2000 logger for SD
 *
 * This implements a simple logger for limited NMEA2000 data, as required to
 * support Volunteered Geographic Information in the manner recommended by
 * the IHO Crowdsourced Bathymetry Working Group (IHO document B-12).  This
 * uses the timing information from the network to keep time and generate
 * timestamps for data that don't have their own, but uses the GNSS time
 * for position information.  Output is to an SD card using the Arduino-style
 * libraries for access.  Data output is binary, with no expectation that
 * it will ever be read directly by the code here --- storaged and transfer
 * to the data collector will all be binary, with expansion and conversion
 * at the (more capable) application level.  The goal here is simplicity.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping
 * and NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include <stdint.h>
#include <Arduino.h>
#include <SD.h>
#include "N2kLogger.h"
#include "N2kMessages.h"

const int SoftwareVersionMajor = 1; ///< Software major version for the logger
const int SoftwareVersionMinor = 0; ///< Software minor version for the logger
const int SoftwareVersionPatch = 0; ///< Software patch version for the logger

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
    unsigned long t = millis();
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
        // We wrapped the millisecond counter
        diff = rtn.RawElapsed() + (0xFFFFFFFFUL - m_elapsedTimeAtDatum) + 1;
    } else {
        diff = rtn.RawElapsed() - m_elapsedTimeAtDatum + 1;
    }
    double time_now = m_lastDatumTime + diff/1000.0;
    
    rtn.datestamp = m_lastDatumDate;
    if (time_now > 24.0*60.0*60.0) {
        // We've skipped a day
        ++rtn.datestamp;
        time_now -= 24.0*60.0*60.0;
    }
    rtn.timestamp = time_now;
    return rtn;
}

/// Generate a printable version of the reference real-time in the object.
///
/// \return String with a human-readable (non-compact) version of the reference time.

String Timestamp::printable(void) const
{
    String rtn;
    rtn = "R: " + String(m_lastDatumDate) + " days, " +
          String(m_lastDatumTime) + "s, at counter " +
          String(m_elapsedTimeAtDatum) + "ms since boot";
    return rtn;
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
}

/// Generate a printable version of the estimated real time in the TimeDatum.
///
/// \return String with the human-readable (non-compact) version of the real time estimate.

String Timestamp::TimeDatum::printable(void) const
{
    String rtn;
    rtn = "T: " + String(datestamp) + " days, " + String(timestamp) + " s";
    return rtn;
}

/// Default constructor for the logger and message handler.  This essentially just initialises
/// the base class (with the pointer to the NMEA2000 source handler).
///
/// \param source   Pointer to the NMEA2000 object handling the CAN bus interface.

N2kLogger::N2kLogger(tNMEA2000 *source)
: tNMEA2000::tMsgHandler(0, source), m_verbose(false), m_serialiser(nullptr)
{
}

/// Default destructor for the object.  This attempts to take down the output log file cleanly,
/// and then logs the fact in the console log before completing.  This might take a little time
/// under some circumstances, but should happen rarely.

N2kLogger::~N2kLogger(void)
{
    delete m_serialiser;
    if (m_outputLog)
        m_outputLog.close();
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    SDFile console = SD.open("/console.log", FILE_APPEND);
#else
    SDFile console = SD.open("/console.log", FILE_WRITE);
#endif
    
    console.println("Stopped logging under control.");
    console.close();
}

/// Start logging data to a new log file, generating the next log number in sequence that
/// hasn't been used within the current data set.  The numbers of the log files are used
/// starting with zero, so it's possible that the "next" log file has lower number than the
/// current one.  Note that this doesn't attempt to stop logging to the current file, if that's
/// happening, and there's only one pointer held for the file.  Therefore, if the system could
/// already be logging, you should call CloseLogfile() first.

void N2kLogger::StartNewLog(void)
{
    Serial.println("Starting new log ...");
    uint32_t log_num = GetNextLogNumber();
    Serial.print("Log Number: ");
    Serial.println(log_num);
    String filename = MakeLogName(log_num);
    Serial.print("Log Name: ");
    Serial.println(filename);

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    m_console = SD.open("/console.log", FILE_APPEND);
#else
    m_console = SD.open("/console.log", FILE_WRITE);
#endif

    m_outputLog = SD.open(filename, FILE_WRITE);
    if (m_outputLog) {
        m_serialiser = new Serialiser(m_outputLog);
        m_console.println(String("INFO: started logging to ") + filename);
    } else {
        m_serialiser = nullptr;
        m_console.println(String("ERR: Failed to open output log file as ") + filename);
    }
    
    m_console.close();
    
    Serial.println("New log file initialisation complete.");
}

/// Close the current log file, and reset the Serialiser.  This ensures that the output log
/// file is safely closed, and no other object has reference to the file structure used
/// for it.

void N2kLogger::CloseLogfile(void)
{
    delete m_serialiser;
    m_serialiser = nullptr;
    m_outputLog.close();
}

/// Remove a specific log file from the SD card.  The specification of the filename, etc.
/// is abstracted out, so that only the file number is required to remove.  Note that the
/// code here doesn't check that the file exists before attempting to remove it.  Therefore,
/// it is possible that you might receive a failure code either because the file doesn't exist
/// or because the remove failed, and you can't tell the difference.
///
/// \param file_num Logical file number to remove.
/// \return True if the file was successfully removed, otherwise False

boolean N2kLogger::RemoveLogFile(uint32_t file_num)
{
    String filename = MakeLogName(file_num);
    boolean rc = SD.remove(filename);

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    SDFile console = SD.open("/console.log", FILE_APPEND);
#else
    SDFile console = SD.open("/console.log", FILE_WRITE);
#endif

    if (rc) {
        console.println(String("INFO: erased log file ") + file_num + String(" by user command."));
    } else {
        console.println(String("ERR: failed to erase log file ") + file_num + String(" on command."));
    }
    console.close();
    
    return rc;
}

/// Remove all log files on the system.  Note that this includes the file that is currently
/// being written, if there is one ("all" means all).  The logger then automatically starts
/// a new log file immediately.

void N2kLogger::RemoveAllLogfiles(void)
{
    SDFile basedir = SD.open("/logs");
    SDFile entry = basedir.openNextFile();
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    SDFile console = SD.open("/console.log", FILE_APPEND);
#else
    SDFile console = SD.open("/console.log", FILE_WRITE);
#endif

    CloseLogfile();  // All means all ...
    
    long file_count = 0, total_files = 0;
    
    while (entry) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
        String filename = entry.name();
#else
        String filename = String("/logs/") + entry.name();
#endif
        entry.close();
        ++total_files;

        Serial.println("INFO: erasing log file: \"" + filename + "\"");
        
        boolean rc = SD.remove(filename);
        if (rc) {
            console.println(String("INFO: erased log file ") + filename + String(" by user command."));
            ++file_count;
        } else {
            console.println(String("ERR: failed to erase log file ") + filename + String(" by user command."));
        }
        entry = basedir.openNextFile();
    }
    basedir.close();
    console.println(String("INFO: erased ") + file_count + String(" log files of ") + total_files);
    console.close();

    StartNewLog();
}

/// Generate a human-readable version of the logger's version information.  This is a simple
/// major.minor.patch string.
///
/// \return String representation of the versioning information for the logger specifically.

String N2kLogger::SoftwareVersion(void) const
{
    String rtn;
    rtn = String(SoftwareVersionMajor) + "." + String(SoftwareVersionMinor) + "." + String(SoftwareVersionPatch);
    return rtn;
}

/// Implementation of the callback method required by the NMEA2000 handler to take care
/// of messages received on the NMEA2000 bus.  Processing here is simply a matter of getting
/// a good time stamp as quickly as possible, and then handing over parsing of the message to
/// the specialist routines, one per message type.
///
/// \param message  Reference for the NMEA2000 message received

void N2kLogger::HandleMsg(const tN2kMsg& message)
{
    // Everything is going to need a timestamp, so get it once.
    Timestamp::TimeDatum now = m_timeReference.Now();
    
    switch (message.PGN) {
        case 126992UL:  HandleSystemTime(now, message); break;
        case 127257UL:  HandleAttitude(now, message); break;
        case 128267UL:  HandleDepth(now, message); break;
        case 129026UL:  HandleCOG(now, message); break;
        case 129029UL:  HandleGNSS(now, message); break;
        case 130311UL:  HandleEnvironment(now, message); break;
        case 130312UL:  HandleTemperature(now, message); break;
        case 130313UL:  HandleHumidity(now, message); break;
        case 130314UL:  HandlePressure(now, message); break;
        case 130316UL:  HandleExtTemperature(now, message); break;
        default:
            // We ignore all packets, unless we're in verbose mode
            if (m_verbose) {
                Serial.print("DBG: Found, and ignoring, packet ID ");
                Serial.println(message.PGN);
            }
            break;
    }
    
    // Check here whether the log file is getting too big, and cycle to
    // the next file if appropriate.
}

/// Manage the translation and serialisation of the SystemTime message.  This extracts the
/// real time, and serialises, but also uses the reference time to update the local time reference
/// point for interpolation of future time stamps.  There are a variety of different time sources
/// that could generate the SystemTime message, including a local crystal oscillator, which is
/// usually very poorly regulated.  Messages tagged with this reference source are ignored.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleSystemTime(Timestamp::TimeDatum const& t, const tN2kMsg& msg)
{
    unsigned char   SID;
    uint16_t        date;
    double          timestamp;
    tN2kTimeSource  source;

    if (m_verbose)
        Serial.println("DBG: Handling SystemTime packet.");
    
    // Note that we don't attempt to get the time here, since we did
    // that already when the message was received, and we want the lowest
    // latency version of this.  We can retrieve this from [t] as below.
    
    if (ParseN2kSystemTime(msg, SID, date, timestamp, source)) {
        if (source != N2ktimes_LocalCrystalClock) {
            m_timeReference.Update(date, timestamp, t.RawElapsed());
            Serialisable s(sizeof(uint16_t) + sizeof(double) + sizeof(unsigned long) + 1);
            s += date;
            s += timestamp;
            s += t.RawElapsed();
            s += (uint8_t)source;
            m_serialiser->Process(Pkt_SystemTime, s);
            m_console.print("INF: Time update to: ");
            m_console.println(m_timeReference.printable());
        }
    }
}

/// Manage the translation and serialisation of the Attitude NMEA2000 message.  This breaks out the
/// attitude data, and then immediately serialises to file.  The timestamp is also serialised.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleAttitude(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char   SID;
    double          yaw, pitch, roll;

    if (m_verbose)
        Serial.println("DBG: Handling Attitude packet.");
    
    if (ParseN2kAttitude(msg, SID, yaw, pitch, roll)) {       
        Serialisable s(t.SerialisationSize() + 3*sizeof(double));
        t.Serialise(s);
        s += yaw;
        s += pitch;
        s += roll;
        m_serialiser->Process(Pkt_Attitude, s);
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse attitude data packet.");
    }
}

/// Manage the translation and serialisation of the Observed Depth NMEA2000 message.  This breaks out
/// the depth, offset, and maximum depth range, and serialised, along with the timestamp.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleDepth(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char SID;
    double depth, offset, range;

    if (m_verbose)
        Serial.println("DBG: Handling Depth packet.");
    
    if (ParseN2kWaterDepth(msg, SID, depth, offset, range)) {
        Serialisable s(t.SerialisationSize() + 3*sizeof(double));
        t.Serialise(s);
        s += depth;
        s += offset;
        s += range;
        m_serialiser->Process(Pkt_Depth, s);
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse water depth packet.");
    }
}

/// Manage the translation and serialisation of the Course-over-ground/Speed-over-ground
/// NMEA2000 message (which is often an rapid cycle, rather than 1Hz).  This breaks out
/// the COG and SOG values, and serialises directly, along with the timestamp.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleCOG(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char SID;
    tN2kHeadingReference ref;
    double  cog, sog;

    if (m_verbose)
        Serial.println("DBG: Handling COG packet.");
    
    if (ParseN2kCOGSOGRapid(msg, SID, ref, cog, sog)) {
        if (ref == N2khr_true) {
            Serialisable s(t.SerialisationSize() + 2*sizeof(double));
            t.Serialise(s);
            s += cog;
            s += sog;
            m_serialiser->Process(Pkt_COG, s);
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse COG/SOG packet.");
    }
}

/// Manage the translation and serialisation of the GNSS NMEA2000 message.  This provides the
/// primary position information, optionally corrected if any corrections are available.  The code
/// unpacks and serialises all of the information, but also uses the time information to set the
/// time reference information if no other source has been provided before this message.  This
/// should provide at least a rough estimate of time for other packets being serialised.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleGNSS(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char   SID;
    uint16_t        datestamp;
    double          timestamp;
    double          latitude, longitude, altitude;
    tN2kGNSStype    rec_type;
    tN2kGNSSmethod  rec_method;
    unsigned char   nSvs;
    double          hdop, pdop, sep;
    unsigned char   nRefStations;
    tN2kGNSStype    refStationType;
    uint16_t        refStationID;
    double          correctionAge;

    if (m_verbose)
        Serial.println("DBG: Handling GNSS packet.");
    
    if (ParseN2kGNSS(msg, SID, datestamp, timestamp, latitude, longitude, altitude,
                     rec_type, rec_method, nSvs, hdop, pdop, sep, nRefStations,
                     refStationType, refStationID, correctionAge)) {
        Serialisable s(2*sizeof(uint16_t) + 8*sizeof(double) + 5);
        s += datestamp; s += timestamp;
        s += latitude; s += longitude; s += altitude;
        s += (uint8_t)rec_type; s += (uint8_t)rec_method;
        s += nSvs;
        s += hdop; s += pdop;
        s += sep;
        s += nRefStations;
        s += (uint8_t)refStationType;
        s += refStationID;
        s += correctionAge;
        m_serialiser->Process(Pkt_GNSS, s);
        
        if (!m_timeReference.IsValid()) {
            // We haven't yet seen a valid time reference, but the time here is
            // probably OK since it's usually 1Hz.  Therefore we can update if
            // we don't have anything else
            m_timeReference.Update(datestamp, timestamp, t.RawElapsed());
            m_console.print("INF: Time update to: ");
            m_console.print(m_timeReference.printable());
            m_console.println(" from GNSS record.");
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse primary GNSS report packet.");
    }
}

/// Manage the translation and serialisation of the Environment NMEA2000 data message.  This
/// message is officially deprecated, but might still occur.  The code here translates out the temperatue,
/// humidity, and pressure, and the temperature and humidity sources, and then serialises.  Note that
/// it's possible that the temperature could be from any of the sources (including ones that we don't
/// remotely care about), but we log anyway.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleEnvironment(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char SID;
    tN2kTempSource      t_source;
    tN2kHumiditySource  h_source;
    double              temp, humidity, pressure;

    if (m_verbose)
        Serial.println("DBG: Handling environmental packet.");
    
    if (ParseN2kEnvironmentalParameters(msg, SID, t_source, temp, h_source, humidity, pressure)) {
        Serialisable s(t.SerialisationSize() + 3*sizeof(double) + 2);
        t.Serialise(s);
        s += (uint8_t)t_source;
        s += temp;
        s += (uint8_t)h_source;
        s += humidity;
        s += pressure;
        m_serialiser->Process(Pkt_Environment, s);
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse environmental parameters packet.");
    }
}

/// Manage the translation and serialisation of the Temperature NMEA2000 message.  This
/// translates the temperature and source, but only logs the information if it is marked as
/// coming from water (SeaTemperature) or air (OutsideTemperature) to avoid extra information
/// from such things as bait wells and refrigerators.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleTemperature(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char   SID;
    unsigned char   temp_instance;
    tN2kTempSource  t_source;
    double          temp, set_temp;

    if (m_verbose)
        Serial.println("DBG: Handling Temperature packet.");
    
    if (ParseN2kTemperature(msg, SID, temp_instance, t_source, temp, set_temp)) {
        if (t_source == N2kts_SeaTemperature || t_source == N2kts_OutsideTemperature) {
            Serialisable s(t.SerialisationSize() + 1 + sizeof(double));
            t.Serialise(s);
            s += (uint8_t)t_source;
            s += temp;
            m_serialiser->Process(Pkt_Temperature, s);
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse temperature packet.");
    }
}

/// Manage the translation and serialisation of the Humidity NMEA2000 message.  This picks
/// out the humidity information and source, but only serialises if the humidity is from the air
/// (OutsideHumidity) in order to avoid information about in-cabin measurements.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleHumidity(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char       SID;
    unsigned char       humidity_instance;
    tN2kHumiditySource  h_source;
    double              humidity;

    if (m_verbose)
        Serial.println("DBG: Handling Humidity packet.");
    
    if (ParseN2kHumidity(msg, SID, humidity_instance, h_source, humidity)) {
        if (h_source == N2khs_OutsideHumidity) {
            Serialisable s(t.SerialisationSize() + 1 + sizeof(double));
            t.Serialise(s);
            s += (uint8_t)h_source;
            s += humidity;
            m_serialiser->Process(Pkt_Humidity, s);
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse humidity packet.");
    }
}

/// Manage the translation and serialisation of the Pressure NMEA2000 message.  This picks out
/// the pressure and source information, but only serialises if the pressure is from outside (Atmospheric)
/// to avoid information from other sources (e.g., compressed air) getting into the log.
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandlePressure(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char       SID;
    unsigned char       pressure_instance;
    tN2kPressureSource  p_source;
    double              pressure;

    if (m_verbose)
        Serial.println("DBG: Handling Pressure packet.");
    
    if (ParseN2kPressure(msg, SID, pressure_instance, p_source, pressure)) {
        if (p_source == N2kps_Atmospheric) {
            Serialisable s(t.SerialisationSize() + 1 + sizeof(double));
            t.Serialise(s);
            s += (uint8_t)p_source;
            s += pressure;
            m_serialiser->Process(Pkt_Pressure, s);
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse pressure packet.");
    }
}

/// Manage the translation and serialisation of the ExtTemperature NMEA2000 message.
/// This picks out the temperature and source, but only serialises if the temperature is marked
/// as coming from the water (SeaTemperature) or air (OutsideTemperature).
///
/// \param t    Estimate of real time associated with the current message
/// \param msg  NMEA2000 message to be serialised.

void N2kLogger::HandleExtTemperature(Timestamp::TimeDatum const& t, tN2kMsg const& msg)
{
    unsigned char   SID;
    unsigned char   temp_instance;
    tN2kTempSource  t_source;
    double          temp, set_temp;

    if (m_verbose)
        Serial.println("DBG: Handling ExtTemperature packet.");
    
    if (ParseN2kTemperatureExt(msg, SID, temp_instance, t_source, temp, set_temp)) {
        if (t_source == N2kts_SeaTemperature || t_source == N2kts_OutsideTemperature) {
            Serialisable s(t.SerialisationSize() + 1 + sizeof(double));
            t.Serialise(s);
            s += (uint8_t)t_source;
            s += temp;
            m_serialiser->Process(Pkt_Temperature, s);
        }
    } else {
        m_console.println(t.printable() + ": ERR: Failed to parse temperature packet.");
    }
}

/// Generate a logical file number for the next log file to be written.  This operates
/// by walking the current log directory counting files that exist until the upper
/// limit is reached.  The log directory is created if it does not already exist, and any
/// standard file that appears in the wrong location is removed.  If all log files exist,
/// the code re-uses the first file number, over-writing the log file.  The "next" log
/// file is the first file number that is attempted where a prior log file is not existant
/// on SD card.  This means that the "next" log file might have lower logical number
/// than the current or previous one.
///
/// \return Logical file number for the next log file to write.

uint32_t N2kLogger::GetNextLogNumber(void)
{
    if (!SD.exists("/logs")) {
        SD.mkdir("/logs");
    }
    SDFile dir = SD.open("/logs");
    if (!dir.isDirectory()) {
        dir.close();
        SD.remove("/logs");
        SD.mkdir("/logs");
    }
    
    uint32_t lognum = 0;
    while (lognum < 1000) {
        if (SD.exists(MakeLogName(lognum))) {
            ++lognum;
        } else {
            break;
        }
    }
    if (1000 == lognum)
        lognum = 0; // Re-use the first created
    return lognum;
}

/// Generate a string version for a logical file number.  This converts the logical number
/// into a filename that can be used to open the file.  This assumes that the log file
/// directory already exists.
///
/// \param log_num  Logical file number to generate
/// \return String with full path to the log file to create

String N2kLogger::MakeLogName(uint32_t log_num)
{
    String filename("/logs/nmea2000.");
    filename += log_num;
    return filename;
}
