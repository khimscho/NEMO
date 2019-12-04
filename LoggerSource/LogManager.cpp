/*!\file LogManager.cpp
 * \brief Implement log file management primitives
 *
 * This code implements the functionality to manage log files on the SD card in the logger.  This
 * includes writing to the files (including the console), making new files, removing old files, etc.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include "LogManager.h"

namespace logger {

Manager::Manager(void)
{
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    m_consoleLog = SD.open("/console.log", FILE_APPEND);
#else
    m_consoleLog = SD.open("/console.log", FILE_WRITE);
#endif
}

Manager::~Manager(void)
{
    if (m_outputLog)
        m_outputLog.close();
    
    m_consoleLog.println("INFO: shutting down log manager under control.");
    m_consoleLog.close();
}

/// Start logging data to a new log file, generating the next log number in sequence that
/// hasn't been used within the current data set.  The numbers of the log files are used
/// starting with zero, so it's possible that the "next" log file has lower number than the
/// current one.  Note that this doesn't attempt to stop logging to the current file, if that's
/// happening, and there's only one pointer held for the file.  Therefore, if the system could
/// already be logging, you should call CloseLogfile() first.

void Manager::StartNewLog(void)
{
    Serial.println("Starting new log ...");
    uint32_t log_num = GetNextLogNumber();
    Serial.println(String("Log Number: ") + log_num);
    String filename = MakeLogName(log_num);
    Serial.println(String("Log Name: ") + filename);

    m_outputLog = SD.open(filename, FILE_WRITE);
    if (m_outputLog) {
        m_serialiser = new Serialiser(m_outputLog);
        m_console.println(String("INFO: started logging to ") + filename);
    } else {
        m_serialiser = nullptr;
        m_console.println(String("ERR: Failed to open output log file as ") + filename);
    }
    
    m_consoleLog.flush();
    
    Serial.println("New log file initialisation complete.");
}

/// Close the current log file, and reset the Serialiser.  This ensures that the output log
/// file is safely closed, and no other object has reference to the file structure used
/// for it.

void Manager::CloseLogfile(void)
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

bool Manager::RemoveLogFile(uint32_t file_num)
{
    String filename = MakeLogName(file_num);
    boolean rc = SD.remove(filename);

    if (rc) {
        m_consoleLog.println(String("INFO: erased log file ") + file_num +
                             String(" by user command."));
    } else {
        m_consoleLog.println(String("ERR: failed to erase log file ") + file_num +
                             String(" on command."));
    }
    m_consoleLog.flush();
    
    return rc;
}

/// Remove all log files on the system.  Note that this includes the file that is currently
/// being written, if there is one ("all" means all).  The logger then automatically starts
/// a new log file immediately.

void Manager::RemoveAllLogfiles(void)
{
    SDFile basedir = SD.open("/logs");
    SDFile entry = basedir.openNextFile();

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
            m_consoleLog.println(String("INFO: erased log file ") + filename +
                                 String(" by user command."));
            ++file_count;
        } else {
            m_consoleLog.println(String("ERR: failed to erase log file ") + filename +
                                 String(" by user command."));
        }
        entry = basedir.openNextFile();
    }
    basedir.close();
    m_consoleLog.println(String("INFO: erased ") + file_count + String(" log files of ")
                         + total_files);
    m_consoleLog.flush();

    StartNewLog();
}

/// Record a packet into the current output file, and check on size (making a new file if
/// required).
///
/// \param pktID    Reference number to save with the packet
/// \param data Serialisable or derived object with data to write

void Manager::Record(PacketIDs pktID, Serialisable& data)
{
    m_serialiser->Process((uint32_t)pktID, data);
    if (m_outputLog.size() > MAX_LOG_FILE_SIZE) {
        m_consoleLog.println(String("INFO: Cycling to next log file after ") + m_outputLog.size() + " B to current log file.");
        m_consoleLog.flush();
        CloseLogfile();
        StartNewLog();
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

uint32_t Manager::GetNextLogNumber(void)
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

String Manager::MakeLogName(uint32_t log_num)
{
    String filename("/logs/nmea2000.");
    filename += log_num;
    return filename;
}

}
