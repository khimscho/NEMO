/* \file N2kLogger.cpp
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
 * 2019-08-25.
 */

#include <stdint.h>
#include <Arduino.h>
#include <SD.h>
#include "N2kLogger.h"
#include "N2kMessages.h"

const int SoftwareVersionMajor = 1;
const int SoftwareVersionMinor = 0;
const int SoftwareVersionPatch = 0;

Timestamp::Timestamp(void)
{
    m_lastDatumTime = -1.0; // So !IsValid() == true
}

void Timestamp::Update(uint16_t date, double timestamp)
{
    unsigned long t = millis();
    m_lastDatumDate = date;
    m_lastDatumTime = timestamp;
    m_elapsedTimeAtDatum = t;
}

void Timestamp::Update(uint16_t date, double timestamp, unsigned long ms_counter)
{
    m_lastDatumDate = date;
    m_lastDatumTime = timestamp;
    m_elapsedTimeAtDatum = ms_counter;
}

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

String Timestamp::printable(void) const
{
    String rtn;
    rtn = "R: " + String(m_lastDatumDate) + " days, " +
          String(m_lastDatumTime) + "s, at counter " +
          String(m_elapsedTimeAtDatum) + "ms since boot";
    return rtn;
}

void Timestamp::TimeDatum::Serialise(Serialisable& s) const
{
    s += datestamp;
    s += timestamp;
}

String Timestamp::TimeDatum::printable(void) const
{
    String rtn;
    rtn = "T: " + String(datestamp) + " days, " + String(timestamp) + " s";
    return rtn;
}

N2kLogger::N2kLogger(tNMEA2000 *source)
: tNMEA2000::tMsgHandler(0, source), m_verbose(false), m_serialiser(nullptr)
{
}

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

void N2kLogger::CloseLogfile(void)
{
    delete m_serialiser;
    m_serialiser = nullptr;
    m_outputLog.close();
}

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

String N2kLogger::SoftwareVersion(void) const
{
    String rtn;
    rtn = String(SoftwareVersionMajor) + "." + String(SoftwareVersionMinor) + "." + String(SoftwareVersionPatch);
    return rtn;
}

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

String N2kLogger::MakeLogName(uint32_t log_num)
{
    String filename("/logs/nmea2000.");
    filename += log_num;
    return filename;
}
