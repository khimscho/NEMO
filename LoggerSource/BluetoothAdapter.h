/*! \file BluetoothAdapter.h
 *  \brief Provide veneer for different Bluetooth connectivity modules.
 *
 *  The logger design can have multiple different Bluetooth modules for
 *  connectivity to the data collection app, all with different support
 *  libraries.  The goal here is to provide a thin interface that allows
 *  the code to use any of them (and control the compilation appropriate
 *  to the implementation system).
 *
 *  Copyright (c) 2019, University of New Hampshire, Center for Coastal and
 *  Ocean Mapping.  All Rights Reserved.
 *
 */

#ifndef __BLUETOOTH_ADAPTER_H__
#define __BLUETOOTH_ADAPTER_H__

#include "Arduino.h"

/// \class BluetoothAdapter
/// \brief Interface class for the bluetooth module

class BluetoothAdapter {
public:
    virtual ~BluetoothAdapter() {}
    
    void AdvertiseAs(String const& name);
    void IdentifyAs(String const& identifier);
    
    String LoggerIdentifier(void);
    
    bool IsConnected(void);
    bool DataAvailable(void);
    String ReceivedString(void);
    void WriteString(String const& data);
    void WriteByte(int b);
    
private:
    virtual void setIdentificationString(String const& identifier) = 0;
    virtual String getIdentificationString(void) const = 0;
    virtual void setAdvertisingName(String const& name) = 0;
    virtual String getAdvertisingName(void) const = 0;
    virtual bool isConnected(void) = 0;
    virtual int dataCount(void) = 0;
    virtual String readBuffer(void) = 0;
    virtual void writeBuffer(String const& data) = 0;
    virtual void writeByte(uint8_t b) = 0;
};

/// \class BluetoothFactory
/// \brief Generate the appropriate Bluetooth object for the system

class BluetoothFactory {
public:
    BluetoothAdapter *Create(void);
};

#endif
