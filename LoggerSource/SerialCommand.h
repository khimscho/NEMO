/*! \file SerialCommand.h
 *  \brief Interface for command line processing for the application
 *
 *  The logger takes commands on the serial input line from the user application (or
 *  the user if it's connected to a terminal).  This code generates the interface for
 *  executing those commands.
 *
 *  Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *  All Rights Reserved.
 */

#ifndef __SERIAL_COMMAND_H__
#define __SERIAL_COMMAND_H__

#include <Arduino.h>
#include "N2kLogger.h"
#include "N0183Logger.h"
#include "LogManager.h"
#include "StatusLED.h"
#include "BluetoothAdapter.h"
#include "WiFiAdapter.h"

/// \class SerialCommand
/// \brief Implement a simple ASCII command language for the logger
///
/// The \a SerialCommand object encapsulates a simple command language for the logger,
/// which relies on ASCII commands provided over the Arduino "Serial" object, or (if connected
/// to a client) the Bluetooth LE UART service.  Output from the commands is routed to the
/// Arduino "Serial" UART, and possibly the BLE UART if connected and appropriate.
///
/// The code stores copies of the pointers for the logger and status LED controllers, but does
/// not manage them.  The pointer for the BLE object is managed locally.
///
/// Like most Arduino code, the \a ProcessCommand method needs to be called regularly to
/// check for commands, and execute them.

class SerialCommand {
public:
    /// \brief Default constructor for the command processor
    SerialCommand(nmea::N2000::Logger *canLogger, nmea::N0183::Logger *serialLogger,
                  logger::Manager *logManager, StatusLED *led);
    /// \brief Default destructor
    ~SerialCommand();
    
    /// \brief Poll for commands, and execute if they've been received.
    void ProcessCommand(void);
    
    /// \enum CommandSource
    /// \brief Identify the source of the command being processed
    
    enum CommandSource {
        Serial,     ///< The USB serial connection to a monitoring computer
        Bluetooth,  ///< Bluetooth LE connection from a mobile client
        Wireless    ///< WiFi socket from a client
    };
    
private:
    nmea::N2000::Logger *m_CANLogger;       ///< Pointer for the logger object to use
    nmea::N0183::Logger *m_serialLogger;    ///< Pointer for the NMEA0183 message handler
    logger::Manager     *m_logManager;       ///< Object to write to SD files and console log
    StatusLED           *m_led;     ///< Pointer for the status LED controller
    BluetoothAdapter    *m_ble;     ///< Pointer for the BLE interface
    WiFiAdapter         *m_wifi;    ///< Pointer for the WiFi interface, once it comes up
    
    /// \brief Print the console log on the output stream(s)
    void ReportConsoleLog(CommandSource src);
    /// \brief Walk the log file directory and report the files and their sizes
    void ReportLogfileSizes(CommandSource src);
    /// \brief Report the logger software version string
    void ReportSoftwareVersion(CommandSource src);
    /// \brief Erase one or all of the data log files
    void EraseLogfile(String const& filenum, CommandSource src);
    /// \brief Set the state of the LEDs (primarily for testing)
    void ModifyLEDState(String const& command);
    /// \brief Report the logger's user-specified identification string
    void ReportIdentificationString(CommandSource src);
    /// \brief Set the logger's user-specified identification string
    void SetIdentificationString(String const& identifier);
    /// \brief Set the advertising name for the BLE service
    void SetBluetoothName(String const& name);
    /// \brief Turn on/off verbose information on messages received
    void SetVerboseMode(String const& mode);
    /// \brief Shut down logging for safe power removal
    void Shutdown(void);
    /// \brief Set the WiFi SSID string
    void SetWiFiSSID(String const& ssid);
    /// \brief Get the WiFi SSID string
    void GetWiFiSSID(CommandSource src);
    /// \brief Set the WiFi password string
    void SetWiFiPassword(String const& password);
    /// \brief Get the WiFi password string
    void GetWiFiPassword(CommandSource src);
    /// \brief Turn the WiFi interface either on or off
    void ManageWireless(String const& command, CommandSource src);
    /// \brief Send a log file to the client
    void TransferLogFile(String const& command, CommandSource src);
    
    /// \brief Check for commands, and execute them if found
    void Execute(String const& cmd, CommandSource src);
    
    /// \brief Generate a string on the appropriate output stream
    void EmitMessage(String const& msg, CommandSource src);
};

#endif
