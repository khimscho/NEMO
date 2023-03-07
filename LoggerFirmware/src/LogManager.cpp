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
#include "NVMFile.h"

namespace logger {

// We roll over files at approximately the following size, although we might go over
// a little depending on the size of the last block of data being sent to the file.
// This should be sufficiently large that we don't change files often, but not so large
// that losing a file would be a big problem (and take too long to transfer from the
// logger).  We also have a limited number of file numbers that are used before restarting,
// so we might not use all of the available SD card storage if the log size and maximum
// number of files are set too low.
//    The default configuration is 10MB for files, and 1,000 files, which results in about
// 9.8GB total; if the SD card is bigger than this, it might be appropriate to raise the
// limits (although you'd have to collect data for quite a file to get to this level).

const int MAX_LOG_FILE_SIZE = 10*1024*1024; ///< Maximum size of a single log file before swapping

#ifdef DEBUG_LOG_MANAGER

// When bringing up new hardware designs, it can be problematic if the hardware doesn't
// work, since the logger basically won't run without a log sink.  In order to allow for
// debugging, this implementation of the logger::Manager interface can be used, which
// will provide the same facilities as the full interface, but just throw away all of
// the data, and ignore all commands.

/// \brief Default constuctor
///
/// Construct the log manager.  Normally, the \a StatusLED object would be used to flash
/// one of the LEDs when data is being written to the SD card; in this dummy version,
/// it's simply ignored.
///
/// \param led  Pointer to the LED manager object for the board

Manager::Manager(StatusLED *led)
: m_led(led)
{
    Serial.println("INF: Starting dummy log manager - NO LOGGING WILL BE DONE.");
}

/// \brief Default destructor

Manager::~Manager(void)
{
    // Nothing to be done here
}

/// \brief Close the current log, and start a new one with the next number
///
/// Normally, this would close the current log (if there is one), and start the
/// next one in numeric sequence.  In this dummy version, it simply writes a debug message.

void Manager::StartNewLog(void)
{
    Serial.println("DBG: Call to start new log file");
}

/// \brief Close the current log file
///
/// Normally, this would close the current log file; in this dummy version, it simply writes
/// a debug message.

void Manager::CloseLogfile(void)
{
    Serial.println("DBG: Call to close log file.");
}

/// \brief Remove a log file from the SD card
///
/// Normally, this would remove a specific file from the SD card; in this dummy version,
/// it simply writes a debug message, and always returns True.
///
/// \param file_num The file number to attempt to remove
/// \return True (always)

boolean Manager::RemoveLogFile(const uint32_t file_num)
{
    Serial.printf("DBG: Call to remote log file %d.\n", file_num);
    return true;
}

/// \brief Remove all log files from the SD card
///
/// Normally, this would remove all available log files from the SD card; in this dummy version,
/// it simply writes a debug message.

void Manager::RemoveAllLogfiles(void)
{
    Serial.println("DBG: Call to remove all log files.");
}

/// \brief Count the number of log files on SD card, and provide their file reference numbers
///
/// Normally, this finds all of the log files on the SD card, counts the total number, and returns
/// the internal reference numbers that are being used so that the user can request more details
/// about a specific file later.  In this dummy version, it simply prints a debug message, and returns
/// zero.
///
/// \param filenumbers  Fixed size array (of \a MaxLogFiles) written with the internal reference numbers for the files
/// \return Total number of files available on the SD card

int Manager::CountLogFiles(int filenumbers[MaxLogFiles])
{
    Serial.println("DBG: Call to count log files; returning zero.");
    return 0;
}

/// \brief Determine the size and file name of the specified file
///
/// Normally, this would look up the specified file, and report the filename and size in
/// bytes; in this dummy version, it always reports "UNKNOWN" and a file size of zero.
///
/// \param lognumber    Number of the log to look up
/// \param filename     Reference for where to store the file's name
/// \param filesize     Reference for where to store the file's size

void Manager::EnumerateLogFile(int lognumber, String& filename, int& filesize)
{
    Serial.printf("DBG: Call to enumerate size of logfile %d.\n", lognumber);
    filename = "UNKNOWN";
    filesize = 0;
}

/// \brief Record a packet into the curent log file
///
/// Normally, this would write a \a Serialisable packet of data into the current log file
/// with the ID specified; in this dummy version, it simply prints a debug message.
///
/// \param pktID    Reference ID for the type of data being written
/// \param data     Reference for the data to write to the file

void Manager::Record(PacketIDs pktID, Serialisable const& data)
{
    Serial.printf("DBG: Call to serialise a packet with ID %d.\n", (uint32_t)pktID);
}

/// \brief Getter for the stream on which console information is being written
///
/// Normally, this would return the \a Stream reference for the file being used for console
/// messages (i.e., the permanent log for debug messages); in this dummy version, it simply
/// returns a reference for the hardware \a Serial interface, so messages go on to the
/// standard hardware output.
///
/// \return Reference for the current console output log file

Stream& Manager::Console(void)
{
    return Serial;
}

/// \brief Close the current console log file
///
/// Normally, this would close the console log file being used for permanent debug messages,
/// usually in preparation to stop logging; in this dummy version, it simply writes a 
/// debug message.

void Manager::CloseConsole(void)
{
    Serial.println("DBG: Call to close console log.");
}

/// \brief Output the contents of the console log file
///
/// Normally, this would serialise the console log file on the given stream; in this dummy
/// version, it simply writes a debug message.
///
/// \param output   \a Stream reference on with to write the contents of the console log

void Manager::DumpConsoleLog(Stream& output)
{
    Serial.println("DBG: Call to dump console log to stream.");
}

/// \brief Transfer a log file to a given \a Stream output
///
/// Normally, this is the default log file transfer mechanism (although a specialist version
/// for the WiFi interface also exists), and is used to get data off the logger; in this dummy
/// version, it simply reports a debug mesage with the number of the file requested.
///
/// \param file_num File number to transfer
/// \param output   Reference for a \a Stream on which to write the contents of the file.

void Manager::TransferLogFile(int file_num, Stream& output)
{
    Serial.printf("DBG: Call to transfer log file %d to stream.\n", file_num);
}

#else   // DEBUG_LOG_MANAGER

/// \brief Default constructor.
///
/// Set up for writing to logs, and also for the console log (same idea as a Unix-style
/// syslog).  The system uses the \a StatusLED object to flash one of the LED channels when
/// data is added to the current log file.
///
/// \param led  Pointer to the LED controller for the logger (external owner)

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

/// \brief Default destructor.
///
/// This closes any current output log file, makes a note in the console log, and then shuts
/// everything down.

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
        logger::AlgoRequestStore algstore;
        algstore.SerialiseAlgorithms(m_serialiser);
        logger::MetadataStore metastore;
        metastore.SerialiseMetadata(m_serialiser);
        logger::N0183IDStore filterstore;
        filterstore.SerialiseIDs(m_serialiser);
        logger::ScalesStore scalesstore;
        scalesstore.SerialiseScales(m_serialiser);
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
        String filename = String("/logs/") + entry.name();
        entry.close();
        ++total_files;

        Serial.printf("INFO: erasing log file: \"%s\".\n", filename.c_str());
        
        bool rc = m_storage->Controller().remove(filename);
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
        int dot_position = String(entry.name()).indexOf(".");
        filenumbers[file_count] = String(entry.name()).substring(dot_position+1).toInt();
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
    String filename("/logs/wibl-raw.");
    filename += log_num;
    return filename;
}

/// Output the contents of the system console log to something that implements the Stream
/// interface.
///
/// \param output   Anything Stream-like that supports the write() interface.

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

/// Generic interface to transfer a given log file to any output that supports the Stream
/// interface.  This is not necessarily the fastest way to do this, but it is the most
/// general.  Output protocol is to write the file size as a uint32_t first, then the
/// binary data from the file in order.
///
/// \param file_num Number of the log file to transfer
/// \param output   Anything Stream-like that supports the write() interface.

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
    
    unsigned long start = millis();
    while (f.available()) {
        output.write(f.read());
        ++bytes_transferred;
        if ((bytes_transferred % 1024) == 0) {
            Serial.printf("Transferred %d bytes.\n", bytes_transferred);
        }
    }
    unsigned long end = millis();
    unsigned long duration = (end - start)/1000;
    f.close();
    Serial.println(String("Sent ") + bytes_transferred + " B in " + duration + "s.");
}

#endif

}
