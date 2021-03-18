/*!\file LogManager.cpp
 * \brief Implement log file management primitives
 *
 * This code implements the functionality to manage log files on the SD card in the logger.  This
 * includes writing to the files (including the console), making new files, removing old files, etc.
 *
 * Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string>
#include <list>
#include <stdint.h>
#include <utility>
#include "LogManager.h"
#include "StatusLED.h"
#include "MemController.h"

namespace logger {

const int MAX_LOG_FILE_SIZE = 10*1024*1024; ///< Maximum size of a single log file before swapping

#ifdef DEBUG_LOG_MANAGER
Manager::Manager(StatusLED *led)
: m_led(led)
{
    Serial.println("INF: Starting dummy log manager - NO LOGGING WILL BE DONE.");
}

Manager::~Manager(void)
{
    // Nothing to be done here
}

void Manager::StartNewLog(void)
{
    Serial.println("DBG: Call to start new log file");
}

void Manager::CloseLogfile(void)
{
    Serial.println("DBG: Call to close log file.");
}

boolean Manager::RemoveLogFile(const uint32_t file_num)
{
    Serial.printf("DBG: Call to remote log file %d.\n", file_num);
    return true;
}

void Manager::RemoveAllLogfiles(void)
{
    Serial.println("DBG: Call to remove all log files.");
}

int Manager::CountLogFiles(int filenumbers[MaxLogFiles])
{
    Serial.println("DBG: Call to count log files; returning zero.");
    return 0;
}

void Manager::EnumerateLogFile(int lognumber, String& filename, int& filesize)
{
    Serial.printf("DBG: Call to enumerate size of logfile %d.\n", lognumber);
    filename = "UNKNOWN";
    filesize = 0;
}

void Manager::Record(PacketIDs pktID, Serialisable const& data)
{
    Serial.printf("DBG: Call to serialise a packet with ID %d.\n", (uint32_t)pktID);
}

Stream& Manager::Console(void)
{
    return Serial;
}

void Manager::CloseConsole(void)
{
    Serial.println("DBG: Call to close console log.");
}

void Manager::DumpConsoleLog(Stream& output)
{
    Serial.println("DBG: Call to dump console log to stream.");
}

void Manager::TransferLogFile(int file_num, Stream& output)
{
    Serial.printf("DBG: Call to transfer log file %d to stream.\n", file_num);
}

#else

Manager::Manager(StatusLED *led)
: m_led(led)
{
    m_storage = mem::MemControllerFactory::Create();
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    m_consoleLog = m_storage->Controller().open("/console.log", FILE_APPEND);
#else
    m_consoleLog = m_storage->Controller().open("/console.log", FILE_WRITE);
#endif
    m_consoleLog.println("info: booted logger, appending to console log.");
    m_consoleLog.flush();
    Serial.println("info: started console log.");
}

Manager::~Manager(void)
{
    if (m_outputLog)
        m_outputLog.close();
    
    m_consoleLog.println("INFO: shutting down log manager under control.");
    m_consoleLog.close();
    m_storage->Stop();
    delete m_storage;
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

    m_outputLog = m_storage->Controller().open(filename, FILE_WRITE);
    if (m_outputLog) {
        m_serialiser = new Serialiser(m_outputLog);
        m_consoleLog.println(String("INFO: started logging to ") + filename);
    } else {
        m_serialiser = nullptr;
        m_consoleLog.println(String("ERR: Failed to open output log file as ") + filename);
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
    boolean rc = m_storage->Controller().remove(filename);

    if (rc) {
        m_consoleLog.printf("INFO: erased log file %d by user command.\n", file_num);
    } else {
        m_consoleLog.printf("ERR: failed to erase log file %d on user command.\n", file_num);
    }
    m_consoleLog.flush();
    
    return rc;
}

/// Remove all log files on the system.  Note that this includes the file that is currently
/// being written, if there is one ("all" means all).  The logger then automatically starts
/// a new log file immediately.

void Manager::RemoveAllLogfiles(void)
{
    File basedir = m_storage->Controller().open("/logs");
    File entry = basedir.openNextFile();

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

        Serial.printf("INFO: erasing log file: \"%s\".\n", filename.c_str());
        
        boolean rc = m_storage->Controller().remove(filename);
        if (rc) {
            m_consoleLog.printf("INFO: erased log file \"%s\" by user command.\n", filename.c_str());
            ++file_count;
        } else {
            m_consoleLog.printf("ERR: failed to erase log file \"%s\" by user command.\n", filename.c_str());
        }
        entry = basedir.openNextFile();
    }
    basedir.close();
    m_consoleLog.printf("INFO: erased %ld log files of %ld.\n", file_count, total_files);
    m_consoleLog.flush();

    StartNewLog();
}

/// Count the number of log files on the SD card, so that the client can enumerate them
/// and report to the user.
///
/// \param filenumbers  (Out) Array of the log file numbers on card
/// \return Number of files on the SD card

int Manager::CountLogFiles(int filenumbers[MaxLogFiles])
{
    int file_count = 0;
    
    File logdir = m_storage->Controller().open("/logs");
    File entry = logdir.openNextFile();
    while (entry) {
        filenumbers[file_count] = String(entry.name()).substring(15).toInt();
        ++file_count;
        entry.close();
        entry = logdir.openNextFile();
    }
    logdir.close();
    return file_count;
}

/// Make a list of all of the files that exist on the SD card in the log directory, along with their
/// sizes.  This is generally used to work out which files can be transferred to the client application.
///
/// \param lognumber  Number of the file to look up
/// \param filename   Name of the file
/// \param filesize   Size of the specified file in bytes (or -1 if file doesn't exist)

void Manager::EnumerateLogFile(int lognumber, String& filename, int& filesize)
{
    filename = MakeLogName(lognumber);
    File f = m_storage->Controller().open(filename);
    if (f) {
        filesize = f.size();
    } else {
        filesize = -1;
    }
}

/// Record a packet into the current output file, and check on size (making a new file if
/// required).
///
/// \param pktID    Reference number to save with the packet
/// \param data Serialisable or derived object with data to write

void Manager::Record(PacketIDs pktID, Serialisable const& data)
{
    m_serialiser->Process((uint32_t)pktID, data);
    m_led->TriggerDataIndication();
    if (m_outputLog.size() > MAX_LOG_FILE_SIZE) {
        m_consoleLog.printf("INFO: Cycling to next log file after %d B to current log file.\n", m_outputLog.size());
        m_consoleLog.flush();
        CloseLogfile();
        StartNewLog();
    }
}

Stream& Manager::Console(void)
{
    return m_consoleLog;
}

void Manager::CloseConsole(void)
{
    m_consoleLog.close();
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
 
    if (!m_storage->Controller().exists("/logs")) {
        m_storage->Controller().mkdir("/logs");
    }
    File dir = m_storage->Controller().open("/logs");
    if (!dir.isDirectory()) {
        dir.close();
        m_storage->Controller().remove("/logs");
        m_storage->Controller().mkdir("/logs");
    }
    
    uint32_t lognum = 0;
    while (lognum < MaxLogFiles) {
        if (m_storage->Controller().exists(MakeLogName(lognum))) {
            ++lognum;
        } else {
            break;
        }
    }
    if (MaxLogFiles == lognum)
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

void Manager::DumpConsoleLog(Stream& output)
{
    m_consoleLog.close();
    m_consoleLog = m_storage->Controller().open("/console.log", FILE_READ);
    while (m_consoleLog.available()) {
        output.write(m_consoleLog.read());
    }
    m_consoleLog.close();
    m_consoleLog = m_storage->Controller().open("/console.log", FILE_APPEND);
}

void Manager::TransferLogFile(int file_num, Stream& output)
{
    String filename(MakeLogName(file_num));
    Serial.println("Transferring file: " + filename);
    File f = m_storage->Controller().open(filename, FILE_READ);
    uint32_t bytes_transferred = 0;
    uint32_t file_size = f.size();
    
    // We need to send the file size first, so that the other end knows when to stop
    // listening, and go back to ASCII mode.
    output.write((const uint8_t*)&file_size, sizeof(uint32_t));
    
    while (f.available()) {
        output.write(f.read());
        ++bytes_transferred;
    }
    f.close();
    Serial.println(String("Sent ") + bytes_transferred + " B");
}

#endif

}
