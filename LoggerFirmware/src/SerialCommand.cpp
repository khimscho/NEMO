/*! \file SerialCommand.cpp
 * \brief Implementation for command line processing for the application
 *
 * The logger takes commands on the serial input line from the user application (or
 * the user if it's connected to a terminal).  This code generates the implementation for executing
 * executing those commands.
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

#include <Arduino.h>
#include "BluetoothAdapter.h"
#include "WiFiAdapter.h"
#include "SerialCommand.h"
#include "IncrementalBuffer.h"
#include "OTAUpdater.h"
#include "Configuration.h"
#include "HeapMonitor.h"
#include "NVMFile.h"

const uint32_t CommandMajorVersion = 1;
const uint32_t CommandMinorVersion = 2;
const uint32_t CommandPatchVersion = 0;

/// Default constructor for the SerialCommand object.  This stores the pointers for the logger and
/// status LED controllers for reference, and then generates a BLE service object.  This turns on
/// advertising for the BLE UART service, allowing for data to the sent and received over the air.
///
/// \param CANLogger    Pointer to the NMEA2000 data source
/// \param serialLogger Pointer to the NMEA0183 data source
/// \param logManager   Pointer to the object used to access the log files
/// \param led          Pointer to the status LED controller object

SerialCommand::SerialCommand(nmea::N2000::Logger *CANLogger, nmea::N0183::Logger *serialLogger,
                             logger::Manager *logManager, StatusLED *led)
: m_CANLogger(CANLogger), m_serialLogger(serialLogger), m_logManager(logManager), m_led(led), m_echoOn(true), m_passThrough(false)
{
    logger::HeapMonitor heap;
    bool boot_ble;

    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_BOOT_BLE_B, boot_ble);

    Serial.printf("DBG: Before SerialCommand setup, heap free = %d B\n", heap.CurrentSize());

    m_serialBuffer.ResetLength(1024);

    if (boot_ble) {
        m_ble = BluetoothFactory::Create();
        m_ble->Startup();
        
        Serial.printf("DBG: After BLE start, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
    } else {
        // If we're not booting it at the start, then we're not even going to make the device controller.
        m_ble = nullptr;
    }

    // WiFi always gets created, since we'll need it eventually to get data off.  The only question is
    // whether it gets started when the system first somes up or not.
    m_wifi = WiFiAdapterFactory::Create();

    Serial.printf("DBG: After WiFi interface create, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());

    if (!boot_ble) {
        if (m_wifi->Startup()) {
            Serial.printf("DBG: After WiFi interface start, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());

            bool start_bridge;
            logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_BRIDGE_B, start_bridge);
            if (start_bridge) {
                m_bridge = new nmea::N0183::PointBridge();

                Serial.printf("DBG: After UDP bridge start, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
            } else {
                m_bridge = nullptr;
            }
        } else {
            Serial.printf("ERR: Failed to start WiFi interface.\n");
        }
    }
}

/// Default destructor for the SerialCommand object.  This deallocates the BLE service object,
/// which will force a disconnect.  Note that the pointer references for the logger and status LED
/// objects are not deallocated.

SerialCommand::~SerialCommand()
{
    delete m_bridge;
    delete m_wifi;
    delete m_ble;
}

/// Generate a copy of the console log (/console.log in the root directory of the attached SD card)
/// on the output serial stream, and the BLE stream if there is something connected to it.  Sending
/// things to the BLE stream can be slow since there needs to be a delay between packets to avoid
/// congestion on the stack.
///
/// \param src  Stream by which the command arrived

void SerialCommand::ReportConsoleLog(CommandSource src)
{
    switch (src) {
        case CommandSource::SerialPort:
            Serial.println("*** Current console log start.");
            m_logManager->DumpConsoleLog(Serial);
            Serial.println("*** Current console log end.");
            break;
        case CommandSource::BluetoothPort:
            Serial.println("ERR: cannot output console log on BLE.");
            break;
        case CommandSource::WirelessPort:
            m_logManager->DumpConsoleLog(m_wifi->Client());
            break;
        default:
            Serial.println("ERR: command source not recognised.");
            break;
    }
}

/// Report the sizes of all of the log files that exist in the /logs directory on
/// the attached SD card.  This incidentally reports all of the log files that are
/// ready for transfer.  The report goes out onto the Stream associated with
/// the source of the command.
///
/// \param src  Stream by which the command arrived

void SerialCommand::ReportLogfileSizes(CommandSource src)
{
    EmitMessage("Current log file sizes:\n", src);
    
    int     filenumber[logger::MaxLogFiles];
    int     file_count = m_logManager->CountLogFiles(filenumber);
    String  filename;
    int     filesize;

    for (int f = 0; f < file_count; ++f) {
        m_logManager->EnumerateLogFile(filenumber[f], filename, filesize);
        String line = String("    ") + filename + " " + filesize + " B\n";
        EmitMessage(line, src);
    }
}

/// Report the version string for the current logger implementation.  This also goes out on
/// the BLE interface if there's a client connected.
///
/// \param src  Stream by which the command arrived

void SerialCommand::ReportSoftwareVersion(CommandSource src)
{
    if (m_CANLogger != nullptr) {
        EmitMessage(String("NMEA2000: ") + m_CANLogger->SoftwareVersion() + String("\n"), src);
    }
    if (m_serialLogger != nullptr) {
        EmitMessage(String("NMEA0183: ") + m_serialLogger->SoftwareVersion() + String("\n"), src);
    }
}

/// Erase one, or all, of the log files currently on the SD card.  If the string is "all", all
/// files are removed, including the current one (i.e., the logger stops, removes the file,
/// then re-starts a new one).  Otherwise the string is converted to an integer, and that
/// file is removed if it exists.  Confirmation messages are send out to serial and BLE if
/// there is a client connected.
///
/// \param filenum  String representation of the file number of the log file to remove, or "all"
/// \param src      Stream by which the command arrived

void SerialCommand::EraseLogfile(String const& filenum, CommandSource src)
{
    if (filenum == "all") {
        EmitMessage("Erasing all log files ...\n", src);
        m_logManager->RemoveAllLogfiles();
        EmitMessage("All log files erased.\n", src);
    } else {
        long file_num;
        file_num = filenum.toInt();
        EmitMessage(String("Erasing log file ") + file_num + String("\n"), src);
        if (m_logManager->RemoveLogFile(file_num)) {
            String msg = String("Log file ") + file_num + String(" erased.\n");
            EmitMessage(msg, src);
        } else {
            String msg = String("Failed to erase log file ") + file_num + "\n";
            EmitMessage(msg, src);
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
    } else if (command == "data") {
        m_led->TriggerDataIndication();
    } else {
        Serial.println("ERR: LED status command not recognised.");
    }
}

/// Report the identification string that the user set for the logger.  This is expected to be
/// a unique ID that can be used to identify the logger, but the firmware does not enforce any
/// particular structure for the string.  The output goes to the BLE UART service if there is a
/// client connected.
///
/// \param src      Stream by which the command arrived

void SerialCommand::ReportIdentificationString(CommandSource src)
{
    if (src == CommandSource::SerialPort) EmitMessage("Module identification string: ", src);
    String id;
    if (logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_MODULEID_S, id)) {
        EmitMessage(id + "\n", src);
    } else {
        EmitMessage("UNKNOWN\n", src);
    }
}

/// Set the identification string for the logger.  This is expected to be a unique ID that can be
/// used to identify the logger, but the firmware does not enforce any particular structure for
/// the string, simply recording whatever is provided.  The string is persisted in non-volatile
/// memory on the board so that it is always available.
///
/// \param identifier   String to use to identify the logger

void SerialCommand::SetIdentificationString(String const& identifier)
{
    if (!logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_MODULEID_S, identifier)) {
            Serial.println("ERR: module identification string file failed to write.");
            return;
    }
    //m_ble->IdentifyAs(identifier);
}

/// Set the advertising name for the Bluetooth LE UART service established by the module on
/// start.  This is persisted in non-volatile memory on the logger so that it comes up the same
/// on every start.
///
/// \param name String to use for the advertising name of the logger's BLE interface

void SerialCommand::SetBluetoothName(String const& name)
{
    if (!logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_BLENAME_S, name)) {
        Serial.println("ERR: module advertising name file failed to write.");
        return;
    }
    //m_ble->AdvertiseAs(name);
}

/// Report the advertising name for the Bluetooth LE UART service established by the module when
/// configured.  This is persisted in non-volatile memory on the logger so that it comes up the
/// same on every start
///
/// \param src  Channel on which to report information

void SerialCommand::ReportBluetoothName(CommandSource src)
{
    String name;
    if (src == CommandSource::SerialPort) {
        EmitMessage("BLE Advertising Name: ", src);
    }
    if (logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BLENAME_S, name)) {
        EmitMessage(name + "\n", src);
    } else {
        EmitMessage("UNSET\n", src);
    }
}

/// Set up for verbose reporting of messages received by the logger from the NMEA2000 bus.  This
/// is useful only for debugging, since it generates a lot of traffic on the serial output stream.  Allowable
/// options are "on" and "off".
///
/// \param mode String with "on" or "off" to configure verbose reporting mode

void SerialCommand::SetVerboseMode(String const& mode)
{
    if (mode == "on") {
        if (m_CANLogger != nullptr) m_CANLogger->SetVerbose(true);
        if (m_serialLogger != nullptr) m_serialLogger->SetVerbose(true);
        if (m_bridge != nullptr) m_bridge->SetVerbose(true);
    } else if (mode == "off") {
        if (m_CANLogger != nullptr) m_CANLogger->SetVerbose(false);
        if (m_serialLogger != nullptr) m_serialLogger->SetVerbose(false);
        if (m_bridge != nullptr) m_bridge->SetVerbose(false);
    } else {
        Serial.println("ERR: verbose mode not recognised.");
    }
}

/// Shut down the logger for safe removal of power (typically as part of the power monitoring)
/// so that the log files are completed before the power goes off.  This code shuts down the
/// current log file, and the console file, and then just busy waits for the power to go down.

void SerialCommand::Shutdown(void)
{
    m_logManager->CloseLogfile();
    Serial.println("info: Stopping under control for powerdown");
    m_logManager->Console().println("info: Stopping under control for powerdown.");
    m_logManager->CloseConsole();
    while (true) {
        delay(1000);
    }
    // Intentionally never completes - this is where the code halts
    // for power-down.
}

/// Specify the SSID string to use for the WiFi interface.
///
/// \param ssid SSID to use for the WiFi, when activated

void SerialCommand::SetWiFiSSID(String const& ssid)
{
    m_wifi->SetSSID(ssid);
}

/// Report the SSID for the WiFi on the output channel(s)
///
/// \param src      Stream by which the command arrived

void SerialCommand::GetWiFiSSID(CommandSource src)
{
    String ssid = m_wifi->GetSSID();
    if (src != CommandSource::BluetoothPort) EmitMessage("WiFi SSID: ", src);
    EmitMessage(ssid + "\n", src);
}

/// Specify the password to use for clients attempting to connect to the WiFi access
/// point offered by the module.
///
/// \param password Pre-shared password to expect from the client

void SerialCommand::SetWiFiPassword(String const& password)
{
    m_wifi->SetPassword(password);
}

/// Report the pre-shared password for the WiFi access point.
///
/// \param src      Stream by which the command arrived

void SerialCommand::GetWiFiPassword(CommandSource src)
{
    String password = m_wifi->GetPassword();
    if (src != CommandSource::BluetoothPort) EmitMessage("WiFi Password: ", src);
    EmitMessage(password + "\n", src);
}

/// Turn the WiFi interface on and off, as required by the user
///
/// \param  command     The remnant of the command string from the user
/// \param  src     Stream by which the command arrived

void SerialCommand::ManageWireless(String const& command, CommandSource src)
{
    if (command == "on") {
        logger::HeapMonitor heap;
        Serial.printf("DBG: Before starting WiFi, heap free = %d B\n", heap.CurrentSize());
        if (m_ble != nullptr) {
            m_ble->Shutdown();
            Serial.printf("DBG: After BLE shutdown, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
        }
        if (m_wifi->Startup()) {
            if (src != CommandSource::BluetoothPort)
                EmitMessage("WiFi started on ", src);
            EmitMessage(m_wifi->GetServerAddress() + "\n", src);
            Serial.printf("DBG: After WiFi startup, heap fee = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
            
            bool start_bridge;
            logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_BRIDGE_B, start_bridge);
            if (start_bridge) {
                if (m_bridge == nullptr)
                    m_bridge = new nmea::N0183::PointBridge();
                Serial.printf("DBG: After UDP bridge start, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
            } else {
                m_bridge = nullptr;
            }
        } else {
            EmitMessage("ERR: WiFi startup failed.\n", src);
            if (m_ble != nullptr) {
                m_ble->Startup();
                Serial.printf("DBG: After BLE re-enabled, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
            }
        }
    } else if (command == "off") {
        logger::HeapMonitor heap;
        Serial.printf("DBG: Before stopping WiFi, heap free = %d B\n", heap.CurrentSize());

        m_wifi->Shutdown();
        EmitMessage("WiFi stopped.\n", src);
        Serial.printf("DBG: After WiFi stopped, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
        if (m_bridge != nullptr) {
            delete m_bridge;
            m_bridge = nullptr;
            Serial.printf("DBG: After UDP bridge stopped, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
        }
        if (m_ble != nullptr) {
            m_ble->Startup();
            Serial.printf("DBG: After BLE re-started, heap free = %d B, delta = %d B\n", heap.CurrentSize(), heap.DeltaSinceLast());
        }
    } else if (command == "accesspoint") {
        m_wifi->SetWirelessMode(WiFiAdapter::WirelessMode::ADAPTER_SOFTAP);
    } else if (command == "station") {
        m_wifi->SetWirelessMode(WiFiAdapter::WirelessMode::ADAPTER_STATION);
    } else {
        Serial.println("ERR: wireless management command not recognised.");
    }
}

/// Send a log file to the client (so long as it isn't on BLE, which is way too slow!).  This sends
/// plain binary 8-bit data to the client, which needs to know how to deal with that.  This
/// generally means it really only works on the WiFi connection.
///
/// \param filenum      Log file number to transfer
/// \param src      Stream by which the command arrived.

void SerialCommand::TransferLogFile(String const& filenum, CommandSource src)
{
    String filename;
    int filesize;
    int file_number = filenum.toInt();
    unsigned long tx_start, tx_end, tx_duration;
    m_logManager->EnumerateLogFile(file_number, filename, filesize);
    switch (src) {
        case CommandSource::SerialPort:
            m_logManager->TransferLogFile(file_number, Serial);
            break;
        case CommandSource::BluetoothPort:
            EmitMessage("ERR: not available.\n", src);
            break;
        case CommandSource::WirelessPort:
            Serial.printf("Transfering \"%s\", total %d bytes.\n", filename.c_str(), filesize);
            tx_start = millis();
            m_wifi->TransferFile(filename, filesize);
            tx_end = millis();
            tx_duration = (tx_end - tx_start) / 1000;
            Serial.printf("Transferred in %lu seconds.\n", tx_duration);
            //m_logManager->TransferLogFile(file_number, m_wifi->Client());
            break;
        default:
            Serial.println("ERR: command source not recognised.");
            break;
    }
}

/// Configure whether to invert the bits on either of the NMEA0183 serial ports, in hardware, prior
/// to reception.   This can be used to compensate for reverse polarity connections on the NMEA0183
/// inputs, which is a common problem in intallation.
///
/// \param params   Parameters string for the command: on | off <port-number>
/// \param src      Channel to report information on (Serial, WiFi, BLE)

void SerialCommand::ConfigureSerialPortInvert(String const& params, CommandSource src)
{
    uint32_t    port;
    bool        invert;
    
    if (m_serialLogger == nullptr) {
        EmitMessage("ERR: NMEA0183 logger disabled, cannot run command.\n", src);
        return;
    }
    if (params.startsWith("on")) {
        invert = true;
        port = params.substring(3).toInt();
    } else if (params.startsWith("off")) {
        invert = false;
        port = params.substring(4).toInt();
    } else {
        EmitMessage("ERR: bad command; Syntax: invert on|off <port>\n", src);
        return;
    }
    m_serialLogger->SetRxInvert(port, invert);
}

/// Set up the baud rate for either of the NMEA0183 serial ports.  By default, this should be
/// 4800 baud, as specified in NMEA0183, but some systems run at different rates, and it's
/// possible that this might need to configured.
///
/// \param params   Parameters for the command: <port-number> <baud-rate>
/// \param src      Channel to report information on (Serial, WiFi, BLE)

void SerialCommand::ConfigureSerialPortSpeed(String const& params, CommandSource src)
{
    uint32_t port;
    String baud_rate_str;
    uint32_t baud_rate;
    logger::Config::ConfigParam channel;
    
    if (m_serialLogger == nullptr) {
        EmitMessage("WARN: NMEA0183 logger disabled, expect no effect from this command.\n", src);
    }
    port = params.substring(0,1).toInt();
    if (port == 1 || port == 2) {
        if (port == 1)
            channel = logger::Config::ConfigParam::CONFIG_BAUDRATE_1_S;
        else
            channel = logger::Config::ConfigParam::CONFIG_BAUDRATE_2_S;
    } else {
        EmitMessage("ERR: serial channel must be in {1,2}.\n", src);
        return;
    }
    baud_rate_str = params.substring(2);
    baud_rate = static_cast<uint32_t>(baud_rate_str.toInt());
    if (baud_rate < 4800 || baud_rate > 115200) {
        EmitMessage("ERR: baud rate implausible; ignoring command.\n", src);
        return;
    }
    logger::LoggerConfig.SetConfigString(channel, baud_rate_str);
    EmitMessage("INFO: speed set, remember to reboot to have this take effect.\n", src);
}

/// General interface to set up whether the various data interfaces and optional modules are actually
/// actively being recorded or used, respecitvely, on the logger.  Interfaces include:
///     nmea2000    on | off                NMEA2000 CAN-bus data
///     nmea0183    on | off                NMEA0183 RS-422 data
///     imu         on | off                MPU-6050 motion sensor data
///     power       on | off                Power monitoring and emergency shutdown
///     sdio        on | off                SD/MMC interface for SD card (otherwise SPI)
///     bridge      on <port-number> | off  UDP->RS-422 bridge (port number for the UDP broadcast packet)
///
/// \param params   Parameters for the command: on|off <optional>
/// \param src      Channel to report information on (Serial, WiFi, BLE)

void SerialCommand::ConfigureLoggers(String const& params, CommandSource src)
{
    String  logger;
    bool    state;
    int     position;

    if (params.startsWith("on")) {
        state = true;
        position = 3;
    } else if (params.startsWith("off")) {
        state = false;
        position = 4;
    } else {
        EmitMessage("ERR: loggers can be configured 'on' or 'off' only.\n", src);
        return;
    }
    logger = params.substring(position);

    if (logger.startsWith("nmea2000")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_NMEA2000_B, state);
    } else if (logger.startsWith("nmea0183")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_NMEA0183_B, state);
    } else if (logger.startsWith("imu")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_MOTION_B, state);
    } else if (logger.startsWith("power")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_POWMON_B, state);
    } else if (logger.startsWith("sdio")) {
#ifndef DEBUG_NEMO30
        if (!state) {
            EmitMessage("ERR: cannot turn off SDIO memory card interface for this logger.\n", src);
            state = true;
        }
#endif
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_SDMMC_B, state);
    } else if (logger.startsWith("bridge")) {
        if (state) {
            // If we're configuring on, then there should also be a port number
            String port = logger.substring(7);
            uint16_t port_number = port.toInt();
            if (port_number < 1024) {
                EmitMessage("ERR: UDP bridge port is not valid.\n", src);
                return;
            }
            logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_BRIDGE_PORT_S, port);
        }
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_BRIDGE_B, state);
    } else {
        EmitMessage("ERR: logger name not recognised.\n", src);
    }
}

/// Set whether the logger echos back characters to the host on the interface so that humans can see
/// what they're typing.
///
/// \param params   Parameters for the command: on | off
/// \param src      Channel on which to report the results (Serial, WiFi, BLE)

void SerialCommand::ConfigureEcho(String const& params, CommandSource src)
{
    if (params.startsWith("on")) {
        EchoOn();
    } else if (params.startsWith("off")) {
        EchoOff();
    } else {
        EmitMessage("ERR: echo can be turned 'on' or 'off' only.\n", src);
    }
}

/// Stealth command to turn on "pass through" mode for the logger, which allows the serial interface host
/// computer to send data through the ESP32's UART0, which then gets sent out the TX1 serial interface as
/// an RS-422 signal.  This can be used either to have the logger act as a transmitter without having to
/// reflash with the hardware simulator firmware, or to act as its own data source by looping back TX1 A/B
/// to one of the RX inputs.  The special mode ends when the user sends the string "passthrough" exactly
/// to the interface.  It's wise to also send "echo off" before and "echo on" afterwards to avoid all of
/// the data the host's sending coming back up the USB line.
///
/// \param params   Parameters for the command: [on] (no parameter => switch back to standard operation)
/// \param src      Channel on which to report the results of the command.

void SerialCommand::ConfigurePassthrough(String const& params, CommandSource src)
{
    if (params == "on") {
        m_passThrough = true;
    } else {
        m_passThrough = false;
    }
    EmitMessage("INF: passthrough mode set to: " + params + "\n", src);
}

/// Provide summary report of all of the configuration parameters being managed by the Confgiuration
/// module.  Some of this information can be determined from other locations, but this might be simpler
/// for some purposes (e.g., getting a synoptic list of all of the configuration parameters to set up
/// a GUI for configuration purposes).
///
/// \param src  Channel on which to report the configuration (Serial, WiFi, BLE)

void SerialCommand::ReportConfiguration(CommandSource src)
{
    bool bin_param;
    String string_param;

    EmitMessage("Configuration Parameters:\n", src);
    EmitMessage("  NMEA0183 Logger: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_NMEA0183_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    EmitMessage("  NMEA2000 Logger: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_NMEA2000_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    EmitMessage("  IMU Logger: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_MOTION_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    EmitMessage("  Power Monitor: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_POWMON_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    EmitMessage("  SDIO-MMC Interface: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_SDMMC_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    EmitMessage("  Boot Radio: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_BOOT_BLE_B, bin_param);
    EmitMessage(bin_param ? "ble\n" : "wifi\n", src);
    EmitMessage("  Bridge UDP: ", src);
    logger::LoggerConfig.GetConfigBinary(logger::Config::ConfigParam::CONFIG_BRIDGE_B, bin_param);
    EmitMessage(bin_param ? "on\n" : "off\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_MODULEID_S, string_param);
    EmitMessage("  Module ID String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BLENAME_S, string_param);
    EmitMessage("  BLE Advertising String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WIFISSID_S, string_param);
    EmitMessage("  WiFi SSID String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WIFIPSWD_S, string_param);
    EmitMessage("  WiFi Password String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WIFIIP_S, string_param);
    EmitMessage("  WiFi IP Address String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_WIFIMODE_S, string_param);
    EmitMessage("  WiFi Mode String: " + string_param + "\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BAUDRATE_1_S, string_param);
    EmitMessage("  Serial Channel 1 Speed: " + string_param + " baud\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BAUDRATE_2_S, string_param);
    EmitMessage("  Serial Channel 2 Speed: " + string_param + " baud\n", src);
    logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BRIDGE_PORT_S, string_param);
    EmitMessage("  Bridge UDP Port: " + string_param + "\n", src);
}

/// Report the current state of heap usage in the system.  This reports the maximmum size of the heap,
/// the current free memory in the heap, the low-water mark (lowest free memory since boot), and the
/// largest available chunk of memory on the heap (allowing for monitoring of fragmentation).
///
/// \param src  Channel on which to report summary (Serial, WiFi, BLE).

void SerialCommand::ReportHeapSize(CommandSource src)
{
    logger::HeapMonitor heap;
    String msg = String("Current Heap: ") + heap.HeapSize() + " B total, free: " + heap.CurrentSize() + " B, low-water: "
                    + heap.LowWater() + " B, biggest chunk: " + heap.LargestBlock() + " B.\n";
    EmitMessage(msg, src);
}

/// Configure which radio engine is brought up on boot.  The ESP32 can do both WiFi and Bluetooth (optionally
/// Low Energy), and can actually do both simultaneously.  However, the heap usage to set up both systems can
/// be sufficiently high that you end up with so little free memory left that all sorts of bad things happen
/// (particuarly that send/receive can quietly fail, potentially with reboots).  In order to ensure that we
/// don't have that happen, the user can choose to bring up one or the other by default when the logger starts
/// (although the WiFi can also be brought up separately later if required).
///
/// \param params   Parameters for the command: ble | wifi
/// \param src      Channel on which to report command results (Serial, WiFi, BLE)

void SerialCommand::ConfigureBootRadio(String const& params, CommandSource src)
{
    if (params.startsWith("ble")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_BOOT_BLE_B, true);
    } else if (params.startsWith("wifi")) {
        logger::LoggerConfig.SetConfigBinary(logger::Config::ConfigParam::CONFIG_BOOT_BLE_B, false);
    } else {
        EmitMessage("ERR: radio can only be 'ble' or 'wifi'.\n", src);
        return;
    }
}

/// Report the list of algorithms that the logger would request that the post-processing applies to the data
/// from this system (e.g., de-duplication of depth, reputation computation, etc.).  There is no obligation
/// for the post-processing to do this, but knowing which to do, and in which order, is useful.  The algorithms
/// are specified directly by the user, and the logger does no checking: it just replicates what the user
/// requires.
///
/// \param src  Channel on which to report command results (Serial, WiFi, BLE)

void SerialCommand::ReportAlgRequests(CommandSource src)
{
    logger::AlgoRequestStore algstore;
    switch (src) {
        case CommandSource::SerialPort:
            algstore.ListAlgorithms(Serial);
            break;
        case CommandSource::BluetoothPort:
            m_ble->WriteString("ERR: Cannot report algorithm requests on BLE.\n");
            break;
        case CommandSource::WirelessPort:
            algstore.ListAlgorithms(m_wifi->Client());
            break;
        default:
            EmitMessage("ERR: request for unknown CommandSource - who are you?\n", src);
            break;
    }
}

/// Set up the list of requests for algorithms to be run on the data at the post-processing stage.
/// Algorithms are specified by a simple string name, and a string of parameters, which can be in
/// any form that the algorithm can parse.
///
/// \param params   Parameters for the algorithm: <alg-name> <alg-params>
/// \param src      Channel on which to report results of command (Serial, WiFi, BLE)

void SerialCommand::ConfigureAlgRequest(String const& params, CommandSource src)
{
    logger::AlgoRequestStore algstore;
    String alg_name, alg_params;
    if (params.startsWith("none")) {
        algstore.ResetList();
        return;
    }
    int split = params.indexOf(' ');
    alg_name = params.substring(0, split);
    alg_params = params.substring(split+1);
    algstore.AddAlgorithm(alg_name, alg_params);
    EmitMessage("INF: added algorithm \"" + alg_name + "\" with parameters \"" + alg_params + "\"\n", src);
}

/// Set the string for additional metadata to be applied to the data when it's converted to GeoJSON
/// in post-processing.  The post-processing code can generate a default "platform" element, but if the user
/// sets up a string here, it can be used to completely replaces this (and provide a lot more information)
/// if required.  The code here does not check that the metadata is valid JSON, and just replicates it into
/// the output data files so that the post-processing code has the option to use it if appropriate.  Note
/// that the JSON should be collapsed into a single line so that there are no \r\n in the data (otherwise
/// it'll get split into multiple WIBL commands, and will not be recognised).
///
/// \param params   Parameters for the command: <arbitrary-text>
/// \param src      Channel on which to report results of the command (Serial, WiFi, BLE)

void SerialCommand::StoreMetadataElement(String const& params, CommandSource src)
{
    logger::MetadataStore metastore;
    metastore.WriteMetadata(params);
    EmitMessage("INO: added metadata element to local configuration.\n", src);
}

/// Report the user-specified string for auxiliary metadata generation, if set.  This metadata
/// element is optional, but if available, is reported verbatuim, without interpretation.
///
/// \param src  Channel on which to report command results (Serial, WiFi, BLE)

void SerialCommand::ReportMetadataElement(CommandSource src)
{
    logger::MetadataStore metastore;
    String metadata = metastore.GetMetadata();
    EmitMessage("Metadata element: |" + metadata + "|\n", src);
}

/// Report the currently configured set of NMEA0183 message IDs that are acceptable for logging.
/// All sentences with a message ID in the list will be written to the log; all others are rejected
/// after reception.
///
/// \param src  Channel on which to report the IDs configured for logging (Serial, WiFi, BLE)

void SerialCommand::ReportNMEAFilter(CommandSource src)
{
    logger::N0183IDStore filter;
    EmitMessage("NMEA0183 message IDs accepted for logging:\n", src);
    switch (src) {
        case CommandSource::SerialPort:
            filter.ListIDs(Serial);
            break;
        case CommandSource::BluetoothPort:
            m_ble->WriteString("ERR: Cannot report NMEA0183 filter specification on BLE.\n");
            break;
        case CommandSource::WirelessPort:
            filter.ListIDs(m_wifi->Client());
            break;
        default:
            EmitMessage("ERR: request for unknown CommandSource - who are you?\n", src);
            break;
    }
}

/// Add to the list of NMEA0183 sentence message IDs that are allowed to be logged to SD card on
/// reception.  Any sentence with a message ID on the list will be logged; all others will be
/// rejected.  Note that the code does not check the syntax of the message IDs, or that they are
/// exactly three characters, etc. --- that's up to the user.  The string is therefore case
/// sensitive.  The special ID "any" will reset the filter back to the default state where
/// everything is logged.  Note that the code does not consider the talker ID, so INGGA and
/// GPGGA will both be logged if GGA is on the list.
///
/// \param params   Parameters for the command: any | <arbitrary-text>
/// \param src      Channel on which to report the results of the command (Serial, WiFi, BLE)

void SerialCommand::AddNMEAFilter(String const& params, CommandSource src)
{
    logger::N0183IDStore filter;
    if (params == "all") {
        filter.ResetFilter();
    } else {
        filter.AddID(params);
    }
}

/// Report the set of scales set for any on-board sensors that record binary data that needs to be
/// scaled to useful units and/or into a float in the first place.  This reports the sensor elements
/// specified in the store as JSON fragments, which should be convertable as usual in any language
/// supporting JSON objects.
///
/// \param src      Channel on which to report the results of the command (Serial, WiFi, BLE)

void SerialCommand::ReportScalesElement(CommandSource src)
{
    logger::ScalesStore scales;
    EmitMessage("Sensor scales for on-board sensors:\n", src);
    switch(src) {
        case CommandSource::SerialPort:
        case CommandSource::WirelessPort:
            EmitMessage(scales.GetScales() + "\n", src);
            break;
        case CommandSource::BluetoothPort:
            m_ble->WriteString("ERR: Cannot report sensor scales on BLE.\n");
            break;
        default:
            EmitMessage("ERR: request for unknown CommandSource - who are you?\n", src);
            break;
    }
}

/// Report the number of files that are available on the SD card for transfer.
///
/// \param src      Channel on which to report the results of the command (Serial, WiFi, BLE)

void SerialCommand::ReportFileCount(CommandSource src)
{
    int filenumber[logger::MaxLogFiles];
    int file_count = m_logManager->CountLogFiles(filenumber);
    EmitMessage(String(file_count) + "\n", src);
}

/// Output a list of known commands, since there are now enough of them to make remembering them
/// all a little difficult.

void SerialCommand::Syntax(CommandSource src)
{
    EmitMessage(String("Command Syntax (V") + CommandMajorVersion + "." + CommandMinorVersion + "." + CommandPatchVersion + "):\n", src);
    EmitMessage("  accept [NMEA0183-ID | all]          Configure which NMEA0183 messages to accept.\n", src);
    EmitMessage("  advertise [bt-name]                 Set BLE advertising name.\n", src);
    EmitMessage("  algorithm [name params | none]      Add (or report) an algorithm request to the cloud processing.\n", src);
    EmitMessage("  configure [on|off logger-name]      Configure individual loggers on/off (or report config).\n", src);
    EmitMessage("  echo on|off                         Control character echo on serial line.\n", src);
    EmitMessage("  erase file-number|all               Remove a specific [file-number] or all log files.\n", src);
    EmitMessage("  filecount                           Report the number of log files currently available for transfer.\n", src);
    EmitMessage("  heap                                Report current free heap size.\n", src);
    EmitMessage("  help|syntax                         Generate this list.\n", src);
    EmitMessage("  uniqueid [logger-name]              Set or report the logger's unique identification string.\n", src);
    EmitMessage("  invert 1|2                          Invert polarity of RS-422 input on port 1|2.\n", src);
    EmitMessage("  led normal|error|initialising|full  [Debug] Set the indicator LED status.\n", src);
    EmitMessage("  log                                 Output the contents of the console log.\n", src);
    EmitMessage("  metadata [platform-specific]        Store or report a platform-specific metadata JSON element.\n", src);
    EmitMessage("  ota                                 Start Over-the-Air update sequence for the logger.\n", src);
    EmitMessage("  password [wifi-password]            Set the WiFi password.\n", src);
    EmitMessage("  radio ble|wifi                      Set the radio to boot on initialisation.\n", src);
    EmitMessage("  restart                             Restart the logger module hardware.\n", src);
    EmitMessage("  scales                              Report any registered sensor-specific scale factors.\n", src);
    EmitMessage("  sizes                               Output list of the extant log files, and their sizes in bytes.\n", src);
    EmitMessage("  speed 1|2 baud-rate                 Set the baud rate for the RS-422 input channels.\n", src);
    EmitMessage("  ssid [wifi-ssid]                    Set the WiFi SSID.\n", src);
    EmitMessage("  steplog                             Close current log file, and move to the next in sequence.\n", src);
    EmitMessage("  stop                                Close files and go into self-loop for power-down.\n", src);
    EmitMessage("  transfer file-number                Transfer log file [file-number] (WiFi and serial only).\n", src);
    EmitMessage("  verbose on|off                      Control verbosity of reporting for serial input strings.\n", src);
    EmitMessage("  version                             Report NMEA0183 and NMEA2000 logger version numbers.\n", src);
    EmitMessage("  wireless on|off|accesspoint|station Control WiFi activity [on|off] and mode [accesspoint|station].\n", src);
}

/// Execute the command strings received from the serial interface(s).  This tests the
/// string for known commands, and passes on the options from the string (if any) to
/// the particular methods used for command implementation.  Note that this interface
/// is pretty fragile, since it doesn't check for partial commands, or differences in case,
/// etc.  The expectation is that commands either come from developers, or through an
/// app, and therefore it shouldn't need to be particularly strong.
///
/// \param cmd  String with the user command to execute.
/// \param src      Stream by which the command arrived

void SerialCommand::Execute(String const& cmd, CommandSource src)
{
    if (cmd == "log") {
        ReportConsoleLog(src);
    } else if (cmd == "sizes") {
        ReportLogfileSizes(src);
    } else if (cmd == "version") {
        ReportSoftwareVersion(src);
    } else if (cmd.startsWith("verbose")) {
        SetVerboseMode(cmd.substring(8));
    } else if (cmd.startsWith("erase")) {
        EraseLogfile(cmd.substring(6), src);
    } else if (cmd == "steplog") {
        m_logManager->CloseLogfile();
        m_logManager->StartNewLog();
    } else if (cmd.startsWith("led")) {
        ModifyLEDState(cmd.substring(4));
    } else if (cmd.startsWith("advertise")) {
        if (cmd.length() == 9) {
            ReportBluetoothName(src);
        } else {
            SetBluetoothName(cmd.substring(10));
        }
    } else if (cmd.startsWith("uniqueid")) {
        if (cmd.length() == 8) {
            ReportIdentificationString(src);
        } else {
            SetIdentificationString(cmd.substring(9));
        }
    } else if (cmd == "stop") {
        Shutdown();
    } else if (cmd.startsWith("ssid")) {
        if (cmd.length() == 4)
            GetWiFiSSID(src);
        else
            SetWiFiSSID(cmd.substring(5));
    } else if (cmd.startsWith("password")) {
        if (cmd.length() == 8)
            GetWiFiPassword(src);
        else
            SetWiFiPassword(cmd.substring(9));
    } else if (cmd.startsWith("wireless")) {
        ManageWireless(cmd.substring(9), src);
    } else if (cmd.startsWith("transfer")) {
        TransferLogFile(cmd.substring(9), src);
    } else if (cmd.startsWith("invert")) {
        ConfigureSerialPortInvert(cmd.substring(7), src);
    } else if (cmd.startsWith("ota")) {
        EmitMessage("Starting OTA update sequence ...\n", src);
        OTAUpdater updater; // This puts the logger into a loop which ends with a full reset
    } else if (cmd.startsWith("configure")) {
        if (cmd.length() == 9) {
            ReportConfiguration(src);
        } else {
            ConfigureLoggers(cmd.substring(10), src);
        }
    } else if (cmd == "restart") {
        ESP.restart();
    } else if (cmd.startsWith("echo")) {
        ConfigureEcho(cmd.substring(5), src);
    } else if (cmd == "heap") {
        ReportHeapSize(src);
    } else if (cmd.startsWith("speed")) {
        ConfigureSerialPortSpeed(cmd.substring(6), src);
    } else if (cmd.startsWith("radio")) {
        ConfigureBootRadio(cmd.substring(6), src);
    } else if (cmd == "passthrough") {
        // Note that this is intentionally not defined in the syntax list, or elsewhere: it
        // really is just for debugging.
        ConfigurePassthrough("on", src);
    } else if (cmd.startsWith("algorithm")) {
        if (cmd.length() == 9) {
            ReportAlgRequests(src);
        } else {
            ConfigureAlgRequest(cmd.substring(10), src);
        }
    } else if (cmd.startsWith("metadata")) {
        if (cmd.length() == 8) {
            ReportMetadataElement(src);
        } else {
            StoreMetadataElement(cmd.substring(9), src);
        }
    } else if (cmd.startsWith("accept")) {
        if (cmd.length() == 6) {
            ReportNMEAFilter(src);
        } else {
            AddNMEAFilter(cmd.substring(7), src);
        }
    } else if (cmd == "scales") {
        ReportScalesElement(src);
    } else if (cmd == "filecount") {
        ReportFileCount(src);
    } else if (cmd == "help" || cmd == "syntax") {
        Syntax(src);
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
        int c = Serial.read();
        if (m_echoOn) Serial.printf("%c", c);
        if (c == '\b') {
            // Delete (or at least backspace) character
            m_serialBuffer.RemoveLastCharacter();
        } else {
            m_serialBuffer.AddCharacter(c);
        }
        if (c == '\n') {
            String cmd(m_serialBuffer.Contents());
            m_serialBuffer.Reset();
            if (m_passThrough) {
                if (cmd.startsWith("passthrough")) {
                    ConfigurePassthrough("off", CommandSource::SerialPort);
                } else {
                    Serial1.print(cmd);
                }
            } else {
                cmd.trim();
                
                Serial.printf("Found console command: \"%s\"\n", cmd.c_str());

                Execute(cmd, CommandSource::SerialPort);
            }
        }
    }
    if (m_ble != nullptr && m_ble->IsConnected() && m_ble->DataAvailable()) {
        String cmd = m_ble->ReceivedString();
        cmd.trim();

        Serial.printf("Found BLE command: \"%s\"\n", cmd.c_str());

        Execute(cmd, CommandSource::BluetoothPort);
    }
    if (m_wifi->IsConnected() && m_wifi->DataAvailable()) {
        String cmd = m_wifi->ReceivedString();
        cmd.trim();
        
        Serial.printf("Found WiFi command: \"%s\"\n", cmd.c_str());
        
        Execute(cmd, CommandSource::WirelessPort);
    }
}

/// Provide an external interface to the shutdown HCF so that we log the fact that the emergency
/// power has been activated, and therefore the system is going down.

void SerialCommand::EmergencyStop(void)
{
    Serial.println("WARN: Emergency power activated, shutting down.");
    m_logManager->Console().println("warning: emergency power activated, shutting down.");
    Shutdown();
}

/// Generate a message on the output stream associated with the source given.
///
/// \param  msg     Message to output
/// \param  src     Source of the input command that generated this message

void SerialCommand::EmitMessage(String const& msg, CommandSource src)
{
    switch (src) {
        case CommandSource::SerialPort:
            Serial.print(msg);
            break;
        case CommandSource::BluetoothPort:
            if (m_ble != nullptr && m_ble->IsConnected())
                m_ble->WriteString(msg);
            break;
        case CommandSource::WirelessPort:
            m_wifi->Client().print(msg);
            break;
        default:
            Serial.println("ERR: command source not recognised.");
            break;
    }
}
