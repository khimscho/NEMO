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
#include <Arduino.h>
#include "N2kLogger.h"
#include "StatusLED.h"
#include "BluetoothAdapter.h"

class SerialCommand {
public:
    SerialCommand(N2kLogger *logger, StatusLED *led);
    ~SerialCommand();
    
    void ProcessCommand(void);
    
private:
    N2kLogger           *m_logger;
    StatusLED           *m_led;
    BluetoothAdapter    *m_ble;
    
    void ReportConsoleLog(void);
    void ReportLogfileSizes(void);
    void ReportSoftwareVersion(void);
    void EraseLogfile(String const& filenum);
    void ModifyLEDState(String const& command);
    void ReportIdentificationString(void);
    void SetIdentificationString(String const& identifier);
    void SetBluetoothName(String const& name);
    void SetVerboseMode(String const& mode);
    
    void Execute(String const& cmd);
};

#endif
