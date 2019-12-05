/*! \file SerialCommand.cpp
 * \brief Implementation for command line processing for the application
 *
 * The logger takes commands on the serial input line from the user application (or
 * the user if it's connected to a terminal).  This code generates the implementation for executing
 * executing those commands.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping
 * and NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include <Arduino.h>
#include <SD.h>
#include "BluetoothAdapter.h"
#include "SerialCommand.h"

/// Default constructor for the SerialCommand object.  This stores the pointers for the logger and
/// status LED controllers for reference, and then generates a BLE service object.  This turns on
/// advertising for the BLE UART service, allowing for data to the sent and received over the air.
///
/// \param logger   Pointer to the logger object to use for access to log files
/// \param led          Pointer to the status LED controller object

SerialCommand::SerialCommand(nmea::N2k::Logger *CANLogger, nmea::N0183::Logger *serialLogger,
                             logger::Manager *logManager, StatusLED *led)
: m_CANLogger(CANLogger), m_serialLogger(serialLogger), m_logManager(logManager), m_led(led)
{
    BluetoothFactory factory;
    m_ble = factory.Create();
}

/// Default destructor for the SerialCommand object.  This deallocates the BLE service object,
/// which will force a disconnect.  Note that the pointer references for the logger and status LED
/// objects are not deallocated.

SerialCommand::~SerialCommand()
{
    delete m_ble;
}

/// Generate a copy of the console log (/console.log in the root directory of the attached SD card)
/// on the output serial stream, and the BLE stream if there is something connected to it.  Sending
/// things to the BLE stream can be slow since there needs to be a delay between packets to avoid
/// congestion on the stack.

void SerialCommand::ReportConsoleLog(void)
{
    Serial.println("*** Current console log file:");
    m_logManager->Console().seek(0);
    while (console.available()) {
        int r = console.read();
        Serial.write(r);
        if (m_ble->IsConnected()) {
            m_ble->WriteByte(r);
            delay(5);
        }
    }
    Serial.println("*** Current console log end.");
}

/// Report the sizes of all of the log files that exist in the /logs directory on
/// the attached SD card.  This incidentally reports all of the log files that are
/// ready for transfer.  The report also goes out on the BLE UART if there's a client
/// connected.

void SerialCommand::ReportLogfileSizes(void)
{
    Serial.println("Current log file sizes:");
    logger::Manager::tFileNumeration files(m_logManager->EnumerateLogFiles());
    auto it = files.begin();
    while (it != files.end()) {
        String line = "   " + it.first() + "  " + it.second() + " B";
        Serial.println(line);
        if (m_ble->IsConnected())
            m_ble->WriteString(line + "\n");
    }
}

/// Report the version string for the current logger implementation.  This also goes out on
/// the BLE interface if there's a client connected.

void SerialCommand::ReportSoftwareVersion(void)
{
    String ver(m_CANLogger->SoftwareVersion());
    Serial.println("Software version: " + ver);
    if (m_ble->IsConnected())
        m_ble->WriteString(ver + "\n");
}

/// Erase one, or all, of the log files currently on the SD card.  If the string is "all", all
/// files are removed, including the current one (i.e., the logger stops, removes the file,
/// then re-starts a new one).  Otherwise the string is converted to an integer, and that
/// file is removed if it exists.  Confirmation messages are send out to serial and BLE if
/// there is a client connected.
///
/// \param filenum  String representation of the file number of the log file to remove, or "all"

void SerialCommand::EraseLogfile(String const& filenum)
{
    if (filenum == "all") {
        Serial.println("Erasing all log files ...");
        m_logManager->RemoveAllLogfiles();
        Serial.println("All log files erased.");
        if (m_ble->IsConnected())
            m_ble->WriteString(F("All log files erased.\n"));
    } else {
        long file_num;
        file_num = filenum.toInt();
        Serial.println(String("Erasing log file ") + file_num);
        if (m_logManager->RemoveLogFile(file_num)) {
            String msg = String("Log file ") + file_num + String(" erased.");
            Serial.println(msg);
            if (m_ble->IsConnected())
                m_ble->WriteString(msg + "\n");
        } else {
            String msg = String("Failed to erase log file ") + file_num;
            Serial.println(msg);
            if (m_ble->IsConnected())
                m_ble->WriteString(msg + "\n");
        }
    }
}

/// Set the state of the status LEDs.  One or more LEDs can be associated with the logger,
/// and a set of status colours and patterns can be set by the logger as it runs to indicate
/// different conditions.  For debugging purposes, this code allows the user to set the state
/// directly.  Allowed states are "normal", "error", "initialising", and "full".
///
/// \param command  Command string for LED status to set

void SerialCommand::ModifyLEDState(String const& command)
{
    if (command == "normal") {
        m_led->SetStatus(StatusLED::Status::sNORMAL);
    } else if (command == "error") {
        m_led->SetStatus(StatusLED::Status::sFATAL_ERROR);
    } else if (command == "initialising") {
        m_led->SetStatus(StatusLED::Status::sINITIALISING);
    } else if (command == "full") {
        m_led->SetStatus(StatusLED::Status::sCARD_FULL);
    } else {
        Serial.println("ERR: LED status command not recognised.");
    }
}

/// Report the identification string that the user set for the logger.  This is expected to be
/// a unique ID that can be used to identify the logger, but the firmware does not enforce any
/// particular structure for the string.  The output goes to the BLE UART service if there is a
/// client connected.

void SerialCommand::ReportIdentificationString(void)
{
    Serial.print("Module identification string: ");
    Serial.println(m_ble->LoggerIdentifier());
    if (m_ble->IsConnected())
        m_ble->WriteString(m_ble->LoggerIdentifier());
}

/// Set the identification string for the logger.  This is expected to be a unique ID that can be
/// used to identify the logger, but the firmware does not enforce any particular structure for
/// the string, simply recording whatever is provided.  The string is persisted in non-volatile
/// memory on the board so that it is always available.
///
/// \param identifier   String to use to identify the logger

void SerialCommand::SetIdentificationString(String const& identifier)
{
    m_ble->IdentifyAs(identifier);
}

/// Set the advertising name for the Bluetooth LE UART service established by the module on
/// start.  This is persisted in non-volatile memory on the logger so that it comes up the same
/// on every start.
///
/// \param name String to use for the advertising name of the logger's BLE interface

void SerialCommand::SetBluetoothName(String const& name)
{
    m_ble->AdvertiseAs(name);
}

/// Set up for verbose reporting of messages received by the logger from the NMEA2000 bus.  This
/// is useful only for debugging, since it generates a lot of traffic on the serial output stream.  Allowable
/// options are "on" and "off".
///
/// \param mode String with "on" or "off" to configure verbose reporting mode

void SerialCommand::SetVerboseMode(String const& mode)
{
    if (mode == "on") {
        m_CANLogger->SetVerbose(true);
    } else if (mode == "off") {
        m_CANLogger->SetVerbose(false);
    } else {
        Serial.println("ERR: verbose mode not recognised.");
    }
}

/// Execute the command strings received from the serial interface(s).  This tests the
/// string for known commands, and passes on the options from the string (if any) to
/// the particular methods used for command implementation.  Note that this interface
/// is pretty fragile, since it doesn't check for partial commands, or differences in case,
/// etc.  The expectation is that commands either come from developers, or through an
/// app, and therefore it shouldn't need to be particularly strong.
///
/// \param cmd  String with the user command to execute.

void SerialCommand::Execute(String const& cmd)
{
    if (cmd == "log") {
        ReportConsoleLog();
    } else if (cmd == "sizes") {
        ReportLogfileSizes();
    } else if (cmd == "version") {
        ReportSoftwareVersion();
    } else if (cmd.startsWith("verbose")) {
        SetVerboseMode(cmd.substring(8));
    } else if (cmd.startsWith("erase")) {
        EraseLogfile(cmd.substring(6));
    } else if (cmd == "steplog") {
        m_logger->CloseLogfile();
        m_logger->StartNewLog();
    } else if (cmd.startsWith("led")) {
        ModifyLEDState(cmd.substring(4));
    } else if (cmd.startsWith("advertise")) {
        SetBluetoothName(cmd.substring(10));
    } else if (cmd.startsWith("identify")) {
        ReportIdentificationString();
    } else if (cmd.startsWith("setid")) {
        SetIdentificationString(cmd.substring(6));
    } else {
        Serial.print("Command not recognised: ");
        Serial.println(cmd);
    }
}

/// User-level routine to get commands from any number of sources, and attempt to execute
/// them.  The system uses the Serial input by defult, but will also check for commands from the
/// Bluetooth LE UART service if there is a client connected.
///
/// This routine has to be executed regularly to keep the processing rate going, since commands
/// will be ignored if this code does not run.  Running this periodically in the loop() is appropriate.

void SerialCommand::ProcessCommand(void)
{
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        
        Serial.print("Found command: \"");
        Serial.print(cmd);
        Serial.println("\"");

        Execute(cmd);
    }
    if (m_ble->IsConnected() && m_ble->DataAvailable()) {
        String cmd = m_ble->ReceivedString();

        Serial.print("Found BLE command: \"");
        Serial.print(cmd);
        Serial.println("\"");

        Execute(cmd);
    }
}
