/*! \file BluetoothAdapter.h
 *  \brief Provide veneer for different Bluetooth connectivity modules.
 *
 *  The logger design can have multiple different Bluetooth modules for
 *  connectivity to the data collection app, all with different support
 *  libraries.  The goal here is to provide a thin interface that allows
 *  the code to use any of them (and control the compilation appropriate
 *  to the implementation system).
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

#ifndef __BLUETOOTH_ADAPTER_H__
#define __BLUETOOTH_ADAPTER_H__

#include "Arduino.h"

/// \class BluetoothAdapter
/// \brief Interface class for the bluetooth module
///
/// There are a number of different Bluetooth implementations that could be used, depending
/// on the hardware implementation.  This object is intended to abstract the hardware details
/// so that the user doesn't need to care.  This does mean, however, that the system is a
/// little more indirect than we might prefer.
///
/// The service generated is a UART-like service using the Nordic de facto standard.  This means
/// that the user can port strings over the radio link.  The interaction is therefore message-based.
///
/// Note that the BluetoothAdapter also has to manage the advertising name for the UART
/// service, and the unique user-ID for the logger, storing in non-volatile memory so that they
/// can be set once when the firmware is flashed, and then persisted over power cycles.
///
/// The system provides a stable public interface, and sub-classes must implement the private
/// interface requirements in order to instantiate.  This allows for common management code
/// in the base class that will apply to all implementations (e.g., logging).

class BluetoothAdapter {
public:
    /// \brief Default destructor so that sub-classes can make their own
    virtual ~BluetoothAdapter() {}

    /// \brief Start the Bluetooth adapter and broadcast beacon
    bool Startup(void);
    /// \brief Shut down the Bluetooth adapter
    void Shutdown(void);
    
    /// \brief Set the advertising name for the Bluetooth UART service.
    void AdvertiseAs(String const& name);
    /// \brief Set the user-unique identification string for the module.
    void IdentifyAs(String const& identifier);
    /// \brief Read the user-unique identification string from the module.
    String LoggerIdentifier(void);
    
    /// \brief Verify whether the interface has been started.
    bool IsStarted(void);
    /// \brief Verify whether a client is connected to the UART service
    bool IsConnected(void);
    /// \brief Determine whether data is available from the client.
    bool DataAvailable(void);
    /// \brief Retrieve the first available string from the client.
    String ReceivedString(void);
    /// \brief Write a string to the client, if it is connected.
    void WriteString(String const& data);
    /// \brief Write a single byte to the client, if it is connected.
    void WriteByte(int b);
    
private:
    /// \brief Sub-class implementation of code to start the interface
    virtual bool start(void) = 0;
    /// \brief Sub-class implementation of the code to stop the interface
    virtual void stop(void) = 0;

    /// \brief Set the non-volatile memory associated with the user-unique identification string
    virtual void setIdentificationString(String const& identifier) = 0;
    /// \brief Read the non-volatile memory associated with the user-unique identification string
    virtual String getIdentificationString(void) const = 0;
    /// \brief Set the Bluetooth UART service advertising name in non-volatile memory
    virtual void setAdvertisingName(String const& name) = 0;
    /// \brief Get the Bluetooth UART service advertising name from non-volatile memory
    virtual String getAdvertisingName(void) const = 0;
    /// \brief Check whether the interface has been started or not
    virtual bool isStarted(void) = 0;
    /// \brief Check whether a client has connected to the UART service
    virtual bool isConnected(void) = 0;
    /// \brief Return a count of data packets available from the recieve buffers
    virtual int dataCount(void) = 0;
    /// \brief Read the first string from the receive buffer
    virtual String readBuffer(void) = 0;
    /// \brief Write a string into the output (Tx) buffer for transmit to the client
    virtual void writeBuffer(String const& data) = 0;
    /// \brief Write a single byte into the output (Tx) buffer for transmit to the client
    virtual void writeByte(uint8_t b) = 0;
};

/// \class BluetoothFactory
/// \brief Generate the appropriate Bluetooth object for the system
///
/// In order to abstract the generation of the Bluetooth interface, each implementation must
/// implement a version of the factory creation method to generate their own interface.  This
/// object provides the prototype for this.

class BluetoothFactory {
public:
    /// \brief Create a Bluetooth adapter according to the prevailing hardware
    static BluetoothAdapter *Create(void);
};

#endif
