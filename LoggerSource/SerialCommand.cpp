/*! \file SerialCommand.cpp
*  \brief Implementation for command line processing for the application
*
*  The logger takes commands on the serial input line from the user application (or
*  the user if it's connected to a terminal).  This code generates the implementation for executing
*  executing those commands.
*
*  Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
*  All Rights Reserved.
*/

#include <Arduino.h>
#include <SD.h>
#include "BluetoothAdapter.h"
#include "SerialCommand.h"

SerialCommand::SerialCommand(N2kLogger *logger, StatusLED *led)
: m_logger(logger), m_led(led)
{
    BluetoothFactory factory;
    m_ble = factory.Create();
}

SerialCommand::~SerialCommand()
{
    delete m_ble;
}

void SerialCommand::ReportConsoleLog(void)
{
    Serial.println("*** Current console log file:");
    SDFile console = SD.open("/console.log");
    if (console) {
        while (console.available()) {
            int r = console.read();
            Serial.write(r);
            if (m_ble->IsConnected()) {
                m_ble->WriteByte(r);
                delay(5);
            }
        }
        console.close();
        Serial.println("*** Current console log end.");
    } else {
        Serial.println("ERR: failed to open console log for reporting.");
        if (m_ble->IsConnected())
            m_ble->WriteString("ERR: failed to open console log for reporting.");
    }
}

void SerialCommand::ReportLogfileSizes(void)
{
    Serial.println("Current log file sizes:");
    SDFile logdir = SD.open("/logs");
    SDFile entry = logdir.openNextFile();
    while (entry) {
        Serial.print("  ");
        Serial.print(entry.name());
        Serial.print("  ");
        Serial.print(entry.size());
        Serial.println(" B");
        entry.close();
        entry = logdir.openNextFile();
    }
    logdir.close();
}

void SerialCommand::ReportSoftwareVersion(void)
{
    String ver(m_logger->SoftwareVersion());
    Serial.println("Software version: " + ver);
    if (m_ble->IsConnected())
        m_ble->WriteString(ver + "\n");
}

void SerialCommand::EraseLogfile(String const& filenum)
{
    if (filenum == "all") {
        Serial.println("Erasing all log files ...");
        m_logger->RemoveAllLogfiles();
        Serial.println("All log files erased.");
        if (m_ble->IsConnected())
            m_ble->WriteString(F("All log files erased.\n"));
    } else {
        long file_num;
        file_num = filenum.toInt();
        Serial.println(String("Erasing log file ") + file_num);
        if (m_logger->RemoveLogFile(file_num)) {
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

void SerialCommand::ReportIdentificationString(void)
{
    Serial.print("Module identification string: ");
    Serial.println(m_ble->LoggerIdentifier());
    if (m_ble->IsConnected())
        m_ble->WriteString(m_ble->LoggerIdentifier());
}

void SerialCommand::SetIdentificationString(String const& identifier)
{
    m_ble->IdentifyAs(identifier);
}

void SerialCommand::SetBluetoothName(String const& name)
{
    m_ble->AdvertiseAs(name);
}

void SerialCommand::SetVerboseMode(String const& mode)
{
    if (mode == "on") {
        m_logger->SetVerbose(true);
    } else if (mode == "off") {
        m_logger->SetVerbose(false);
    } else {
        Serial.println("ERR: verbose mode not recognised.");
    }
}

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
