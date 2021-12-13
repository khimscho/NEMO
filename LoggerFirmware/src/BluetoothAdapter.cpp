/*! \file BluetoothAdapter.cpp
 *  \brief Implement factory method and adapters for BLE modules
 *
 *  This code implements a simple factory method to generate a module
 *  adapter for the currently configured module, and the interface
 *  implementations for the different modules supported.
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
#include "Configuration.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include <queue>

// If compiling on the ESP32, we use the internal Bluetooth modem.

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// We need some UUIDs to specify the server, and the Rx and Tx
// characteristics.  These are from www.uuidgenerator.net

/// UUID for Nordic Semi-style Bluetooth LE UART service
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
/// UUID for Nordic Semi-style Bluetooth LE UART receive characteristic
#define RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
/// UUID for Nordic Semi-style Bluetooth LE UART transmit characteristic
#define TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/// \class ServerCallbacks
/// \brief Handle call-backs from the BLE server (for connection management)
///
/// The ESP32 Bluetooth LE stack provides hooks for connection and disconnection events from
/// the server.  In addition to providing these here, the object also provides a routine to ensure that
/// connection state is monitored, and that the system halts for the stack to go back into advertising
/// mode if the client disconnects.

class ServerCallbacks : public BLEServerCallbacks {
public:
    /// \brief Default constructor
    ///
    /// The default constructor does only basic configuration for defaults on connection status.
    /// The Bluetooth server being used is provided for reference so that the code can force the
    /// receiver back into advertising mode on disconnect.
    ///
    /// \param server   Pointer to the BLE server instance for which we're servicing requests.
    
    ServerCallbacks(BLEServer *server)
    : m_connected(false), m_wasConnected(false), m_server(server)
    {
    }
    
    /// \brief Callback for connection events
    ///
    /// This routine recognises that the client has connected through the internal status variables,
    /// and logs the connection information to serial output for debug.
    ///
    /// \param server   Pointer to the BLE server instance for the connection event
    
    void onConnect(BLEServer *server)
    {
        Serial.println("INFO: connected on BLE.");
        m_connected = true;
    }
    
    /// \brief Callback for disconnect events
    ///
    /// This routine recognises that the client has disconnected through the internal status
    /// variable and logs the disconnect information to serial output for debug.
    ///
    /// \param server   Pointer to the BLE server instance for which we're servicing requests
    
    void onDisconnect(BLEServer *server)
    {
        Serial.println("INFO: disconnected on BLE.");
        m_connected = false;
    }
    
    /// \brief User-level routine for connection management and reporting
    ///
    /// In addition to reporting whether the system is currently connected, this routine manages
    /// connect-disconnect monitoring by keeping track of the previous state of the connection
    /// status the last time it was called, so that it can detect changes to connected, and to
    /// disconnected.  Going into connected state just changes the record; going from connected
    /// to disconnected causes the system to re-start advertising for new clients.
    ///
    /// \return True if the system is currently connected, otherwise False
    
    bool IsConnected(void)
    {
        if (m_connected && !m_wasConnected) {
            // We just got connected for the first time from advertising
            m_wasConnected = true; // Record for next time
        }
        if (!m_connected && m_wasConnected) {
            // If we were connected (i.e., this isn't a double disconnect), then
            // we need to pause for a little to let the stack settle, then start
            // advertising again, and record that we have, definitively, been
            // disconnected.
            delay(500);
            m_server->startAdvertising();
            m_wasConnected = false; // Record for next time
        }
        return m_connected;
    }
    
private:
    bool      m_connected;      ///< Current connection status
    bool      m_wasConnected;   ///< Connection status the last time isConnected() was called
    BLEServer *m_server;        ///< Pointer to the server for which we're handling callbacks
};

/// \class CharCallbacks
/// \brief Provide callbacks for write events on the BLE UART
///
/// When the client system writes on the receive characteristic of the BLE UART service, the
/// service provides a hook for a callback to deal with the data.  This object provides that routine,
/// and user-level methods to determine how many received strings are buffered, and to get strings
/// from the buffer.

class CharCallbacks : public BLECharacteristicCallbacks {
public:
    /// \brief Default (null) constructor
    CharCallbacks(void)
    {
    }
    
    /// \brief Callback for reception of data from the client via the BLE UART
    ///
    /// This method is called by the server when data arrives at the module over the
    /// BLE UART.  The routine just reads all of the data available from the Rx characteristic
    /// and then buffers in a FIFO.
    ///
    /// \param ch   Pointer to the Rx characteristic for the BLE UART
    
    void onWrite(BLECharacteristic *ch)
    {
        std::string s = ch->getValue();
        // We can get a newline character at the end of the string, which we need
        // to remove before buffering, since the other input methods don't maintain
        // this in the output.
        if (s.back() == '\n')
          s.pop_back();
        m_rx.push(s);
    }
    
    /// \brief Count the number of received message strings from the client
    ///
    /// The object buffers all of the strings transmitted from the client in a FIFO.  This
    /// provides a length count.
    ///
    /// \return Number of strings buffered in the FIFO
    
    int buffered(void) const
    {
        return m_rx.size();
    }
    
    /// \brief Get the first string from the FIFO
    ///
    /// This provides the first string from the FIFO, or an empty string if there are no values
    /// buffered.
    ///
    /// \return First string from the FIFO, or an empty string
    
    std::string popBuffer(void)
    {
        std::string rx = m_rx.front();
        m_rx.pop();
        return rx;
    }
    
private:
    std::queue<std::string> m_rx;   ///< FIFO buffer of received strings from the client
};

/// \class ESP32Adapter
/// \brief Implement the BLE interface for the ESP32 built-in
///
/// The ESP32 runs Bluetooth (or WiFi) through one of the two cores while the other is available for
/// user activities.  This object provides an implementation of the BluetoothAdapter abstract interface
/// for this built-in BLE.

class ESP32Adapter : public BluetoothAdapter {
public:
    /// \brief Default constructor
    ///
    /// This initialises the SPI-connected Flash File System (SPIFFS) on the ESP-WROOM-32 module
    /// (which is used for non-volatile storage of advertising name and user-unique ID), then initialises
    /// the BLE server for Nordic Semi-style UART service.
    
    ESP32Adapter(void)
    : m_started(false), m_server(nullptr), m_service(nullptr),
      m_txCharacteristic(nullptr), m_rxCallbacks(nullptr)
    {
        // We generate the service and register call-backs, but don't start the service
        // until we're sure that we need it.
        BLEDevice::init(getAdvertisingName().c_str());
        m_server = BLEDevice::createServer();
        m_serverCallbacks = new ServerCallbacks(m_server);
        m_server->setCallbacks(m_serverCallbacks);
        m_service = m_server->createService(SERVICE_UUID);
        m_txCharacteristic =
            m_service->createCharacteristic(TX_CHAR_UUID,
                                          BLECharacteristic::PROPERTY_NOTIFY);
        m_txCharacteristic->addDescriptor(new BLE2902());
        BLECharacteristic *rxCharacteristic =
            m_service->createCharacteristic(RX_CHAR_UUID,
                                          BLECharacteristic::PROPERTY_WRITE);
        m_rxCallbacks = new CharCallbacks();
        rxCharacteristic->setCallbacks(m_rxCallbacks);
    }
    
    /// \brief Default destructor
    ///
    /// This attempts to stop the BLE service if possible, and then cleans up all of the dynamically
    /// allocated objects that supported it.
    
    ~ESP32Adapter()
    {
        stop();

        // We attempt to clean up, although it's not clear that this works well for
        // the current implementation of the BLE driver, which appears to want to
        // call call-backs after the service has been stopped ...
        delete m_rxCallbacks;
        delete m_serverCallbacks;
        delete m_txCharacteristic;
        delete m_service;
        delete m_server;
    }
    
private:
    bool                m_started;  ///< Flag: interface has been started
    BLEServer           *m_server;  ///< Pointer to the BLE server object
    BLEService          *m_service; ///< Pointer to the UART service
    BLECharacteristic   *m_txCharacteristic;    ///< Transmit characteristic for the BLE UART
    ServerCallbacks     *m_serverCallbacks; ///< Callbacks for connection/disconnection events
    CharCallbacks       *m_rxCallbacks; ///< Callbacks for received data from the client
    
    /// This starts the BLE interface, and registers all of the required call-backs, etc.
    ///
    /// \return True if the interface came up successfully.

    bool start(void)
    {
        // We now need to bring up the BLE server with notification on
        // transmit and call-backs on receive.
        m_service->start();
        m_server->getAdvertising()->start();
        Serial.println("INFO: BLE advertising started.");
        
        m_started = true;
        return true;
    }

    /// Bring down the BLE interface so that we don't have to have it running while the WiFi
    /// runs (there isn't really enough memory to run both and anything else at the same time).
    ///
    
    void stop(void)
    {
        if (m_started) {
            m_server->getAdvertising()->stop();
            m_service->stop();
            m_started = false;
            Serial.println("INFO: BLE advertising stopped.");
        }
    }

    /// \brief Set the user-unique identification string for the module
    ///
    /// This opens a file in the SPIFFS non-volatile memory area on the ESP32 module, and
    /// writes the identification string into it.  No further interpretation is done.
    ///
    /// \param id   String to write into the non-volatile memory
    
    void setIdentificationString(String const& id)
    {
        if (!logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_MODULEID_S, id)) {
            Serial.println("ERR: module identification string file failed to write.");
            return;
        }
    }
    
    /// \brief Get the user-unique identification string from the module
    ///
    /// This opens the SPIFFS non-volatile memory area file that contains the user-unique identification
    /// string for the module, and reports the results.
    ///
    /// \return User-unique identification string for the module
    
    String getIdentificationString(void) const
    {
        String value;
        if (!logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_MODULEID_S, value)) {
            Serial.println("ERR: failed to find identification string file for module.");
            value = "UNKNOWN";
        }
        return value;
    }

    /// \brief Set the BLE advertising name for the server
    ///
    /// In order to allow any particular logger to be identified from the data collection device, each
    /// BLE service has to have a unique name.  This allows the user to set the name to use, which
    /// is then persisted in the SPIFFS non-volatile memory for the future.  No interpretation of
    /// the name is done.
    ///
    /// \param name Advertising name for BLE server connection
    
    void setAdvertisingName(String const& name)
    {
        if (!logger::LoggerConfig.SetConfigString(logger::Config::ConfigParam::CONFIG_BLENAME_S, name)) {
            Serial.println("ERR: module advertising name file failed to write.");
            return;
        }
    }
    
    /// \brief Get the BLE advertising name for the server
    ///
    /// This allows the module to re-read the persisted BLE advertising name from the SPIFFS
    /// non-volatile store, so that the module can initialise the BLE server.
    ///
    /// \return Persisted advertising name for the BLE server
    
    String getAdvertisingName(void) const
    {
        String value;
        if (!logger::LoggerConfig.GetConfigString(logger::Config::ConfigParam::CONFIG_BLENAME_S, value)) {
            Serial.println("ERR: failed to find advertising name string file for module.");
            value = "SB2030";
        }
        return value;
    }

    /// \brief Determine whether the interface has been started
    ///
    /// This checks whether the user has started the interface (after instantiating the class)
    ///
    /// \return True if the interface has been started, otherwise False

    bool isStarted(void)
    {
        return m_started;
    }
    
    /// \brief Determine whether the interface has a client connected
    ///
    /// This checks with the server callbacks whether the system is connected.  This also ensures
    /// the there is a check for a switch to disconnected status, so that BLE advertising can restart.
    ///
    /// \return True if a client is connected, otherwise False
    
    bool isConnected(void)
    {
        return m_serverCallbacks != nullptr && m_serverCallbacks->IsConnected();
    }
    
    /// \brief Determine the number of strings in the reception FIFO
    ///
    /// This uses the characteristic callbacks object to determine how many strings have been
    /// received from the client (if any).
    ///
    /// \return Number of strings waiting in the reception FIFO
    
    int dataCount(void)
    {
        int count;
        if (m_rxCallbacks == nullptr)
            count = 0;
        else
            count = m_rxCallbacks->buffered();
        return count;
    }
    
    /// \brief Read the first string out of the reception FIFO
    ///
    /// This pops the first string from the reception FIFO and returns it, reducing the count of
    /// strings available.
    ///
    /// \return First (earliest in time) string from the reception FIFO, or an empty string.
    
    String readBuffer(void)
    {
        String rtn;
        if (m_rxCallbacks != nullptr) {
            rtn = String(m_rxCallbacks->popBuffer().c_str());
        }
        return rtn;
    }

    /// \brief Write a string into the transmission FIFO
    ///
    /// Schedule a string for transmission to the client.  This sends the string to the buffer, and
    /// signals the client that it exists, but overall transmission and clearance from the other end
    /// depends on the BLE implementation and the client configuration.
    ///
    /// \param data String to schedule for transmission
    
    void writeBuffer(String const& data)
    {
        if (m_txCharacteristic != nullptr) {
            m_txCharacteristic->setValue((uint8_t*)(data.c_str()), data.length());
            m_txCharacteristic->notify();
        }
    }

    /// \brief Write a single byte to the transmission FIFO
    ///
    /// This sends a single byte to the transmission characteristic for onward transmission to the
    /// client.  Since this sends a notification event after every byte, it is probably less efficient than
    /// sending an entire string at a time, and potentially a lot less efficient.
    ///
    /// \param b    Byte to send to the client
    
    void writeByte(uint8_t b)
    {
        if (m_txCharacteristic != nullptr) {
            m_txCharacteristic->setValue(&b, 1);
            m_txCharacteristic->notify();
        }
    }
};

/// \brief Create an ESP32 adapter for the module's inbuilt Bluetooth interface
///
/// This instantiates an ESP32Adapter object, and returns the polymorphic pointer to the base class.
///
/// \return Polymorphic pointer to the ESP32Adapter object

BluetoothAdapter *BluetoothFactory::Create(void)
{
    return new ESP32Adapter();
}

#endif

#if defined(__SAM3X8E__)

// All other architectures (typically the Ardunio Due) are assumed to be
// using an external Bluetooth adapter, in this case the Adafruit Bluefruit Friend SPI.

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

const int BLUEFRUIT_SPI_SCLK = 2;   ///< GPIO pin for the software SPI clock
const int BLUEFRUIT_SPI_MISO = 3;   ///< GPIO pin for the software SPI receive data
const int BLUEFRUIT_SPI_MOSI = 4;   ///< GPIO pin for the software SPI transmit data
const int BLUEFRUIT_SPI_CS = 5;     ///< GPIO pin for the software SPI chip select
const int BLUEFRUIT_SPI_IRQ = 6;    ///< GPIO pin for Bluetooth module interrupt request pin
const int BLUEFRUIT_SPI_RST = 7;    ///< GPIO pin for Bluetooth module reset pin

/// Static global variable for the Stream-able Adafruit Bluetooth adapter library
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCLK,
                             BLUEFRUIT_SPI_MISO,
                             BLUEFRUIT_SPI_MOSI,
                             BLUEFRUIT_SPI_CS,
                             BLUEFRUIT_SPI_IRQ,
                             BLUEFRUIT_SPI_RST);

/// \class AdafruitAdapter
/// \brief Implement the BLE interface for an Adafruit BLE Friend SPI
///
/// Implement the BluetoothAdapter abstract interface for the Adafruit BLE Friend SPI breakout board.
/// Since the breakout board has a library adapter that abstracts sends and receives to a Stream-like
/// interface, the implementation requires just an instantaion of the appropriate object, setting the
/// advertising name, and then passing on requests with the Stream interface.

class AdafruitAdapter : public BluetoothAdapter {
public:
    /// \brief Default constructor
    ///
    /// The constructor simple starts the interface, reads the advertising name from NVM and
    /// then turns off echo and verbose modes.  A reset of the BLE device is done to ensure
    /// that the advertising name is correctly taken into account.  This can result is a significant
    /// delay until the module resets.
    
    AdafruitAdapter(void)
    : m_started(false), m_connected(false)
    {
        if (!ble.begin(true)) {
            return;
        }
        if (!ble.atcommand(F("AT+GAPDEVNAME"), getAdvertisingName().c_str())) {
            return;
        }
        if (!ble.reset(true)) {
            return;
        }
        
        ble.echo(false);
        ble.verbose(false);
        m_started = true;
    }
    
    /// \brief Default destructor
    ///
    /// This turns the BLE module back into command mode (just to make sure), disconnects from the
    /// client, if any, and then shuts down the module, releasing it for any other use.
    
    ~AdafruitAdapter()
    {
        ble.setMode(BLUEFRUIT_MODE_COMMAND);
        ble.disconnect();
        ble.end();
        delete m_paramStore;
    }
    
private:
    bool    m_started;      ///< BLE interface is started and running
    bool    m_connected;    ///< BLE interface has been connected at the other end
    
    /// \brief Implement the interface for writing the user-unique identification string
    ///
    /// This uses the non-volatile memory in the Adafruit Bluetooth LE module to persist
    /// the identification string.
    ///
    /// \param id   User-unique identification string to persist
    
    void setIdentificationString(String const& id)
    {
        if (!logger::LoggerConfig.SetConfigString(CONFIG_MODULEID_S, id)) {
            Serial.println("ERR: failed to write identification string.");
        }
    }
    
    /// \brief Implement the interface for reading the user-unique identification string
    ///
    /// This reads the persisted identification string from the non-volatile memory on
    /// the Adafruit Bluetooth module.
    ///
    /// \return User-unique identification string for the module
    
    String getIdentificationString(void) const
    {
        String value;
        if (!logger::LoggerConfig.GetConfigString(CONFIG_MODULEID_S, value)) {
            Serial.println("ERR: failed to read identification string.");
            value = "UNKNOWN";
        }
        return value;
    }

    /// \brief Implement the interface for setting the Bluetooth server advertising name
    ///
    /// This uses the non-volatile memory in the Adafruit Bluetooth module to persist the
    /// advertising name for the BLE UART service.
    ///
    /// \param name Name to use for advertising the Bluetooth connection
    
    void setAdvertisingName(String const& name)
    {
        if (!logger::LoggerConfig.SetConfigString(CONFIG_BLENAME_S, name)) {
            Serial.println("ERR: failed to write advertising name.");
        }
    }
    
    /// \brief Implement the interface for getting the Bluetooth server advertising name
    ///
    /// The reads the persisted Bluetooth server advertising name string from the non-volatile
    /// memory on the Adafruit Bluetooth module.
    ///
    /// \return Advertising name for the Bluetooth module
    
    String getAdvertisingName(void) const
    {
        String value;
        if (!logger::LoggerConfig.GetConfigString(CONFIG_BLENAME_S, value)) {
            Serial.println("ERR: failed to read advertising name string.");
            value = "UNKNOWN";
        }
        return value;
    }
    
    /// \brief Implement the interface for connection detection
    ///
    /// This checks whether a client is connected to the BLE interface, and switches the
    /// module into data mode if so (i.e., so that you don't inadvertently command the
    /// module to do something with random data, as unlikely as that is).
    ///
    /// \return True if a client is connected, otherwise False
    
    bool isConnected(void)
    {
        bool rc = ble.isConnected();
        if (rc && !m_connected) {
            ble.setMode(BLUEFRUIT_MODE_DATA);
        }
        m_connected = rc;
        return rc;
    }
    
    /// \brief Determine the amount of data available from the client in the receive buffers
    ///
    /// This returns an indicator for the amount of data that's available from the client.  It is
    /// not entirely clear whether this is number of strings, or number of bytes.  In general use
    /// either works.
    ///
    /// \return Count of data available from the client.
    
    int dataCount(void)
    {
        return ble.available();
    }
    
    /// \brief Read the next string from the BLE client
    ///
    /// This recovers the next string from the client, reading until the next newline character.  The
    /// newline character is not transferred into the return value.
    ///
    /// \return Latest message from the BLE client
    
    String readBuffer(void)
    {
        String cmd = ble.readStringUntil('\n');
        return cmd;
    }

    /// \brief Schedule a string for transmission to the client
    ///
    /// This sets up the string provided for onward transmission to the BLE UART client.  When and how
    /// the data is transferred depends on the BLE stack implementation, congestion, etc., but it should
    /// all arrive eventually at the other end.
    ///
    /// \param data String to send to the BLE client
    
    void writeBuffer(String const& data)
    {
        ble.println(data);
    }

    /// \brief Schedule a byte for transmission to the client
    ///
    /// This sets up a single byte for onward transmission to the BLE UART client.  Since this is sending
    /// a byte at a time, it's possible that this might be inefficient with respect to sending a full string at
    /// a time.
    ///
    /// \param b    Byte to send to the client
    
    void writeByte(uint8_t b)
    {
        ble.write(b);
    }
};

/// \brief Create an Adafruit BLE Friend adapter Bluetooth interface
///
/// This instantiates an AdafruitAdapter object, and returns the polymorphic pointer to the base class.
///
/// \return Polymorphic pointer to the AdafruitAdapter object


BluetoothAdapter *BluetoothFactory::Create(void)
{
    return new AdafruitAdapter();
}

#endif

// All of the member functions for the BluetoothAdapter are call-throughs
// to the private member methods that the sub-classes have to define.  This
// adds a little to the costs, but it also allows us to instrument the calls
// in a single location, if required.

/// Start the Bluetooth LE module server
///
/// \return True if the module started, otherwise False

bool BluetoothAdapter::Startup(void)
{
    return start();
}

/// Stop the Bluetooth LE module, cleaning up as required

void BluetoothAdapter::Shutdown(void)
{
    stop();
}

/// Set the Bluetooth LE module server name
///
/// \param name Name to set for Bluetooth server advertising

void BluetoothAdapter::AdvertiseAs(String const& name)
{
    setAdvertisingName(name);
}

/// Set the user-unique identification string to use for identifying the logger when taking data
/// files onto the data capture equipment.  This should be a unique string, but the module
/// accepts what's provided by the programming interface as gospel.
///
/// \param identifier   User-unique identification string to use for the module

void BluetoothAdapter::IdentifyAs(String const& identifier)
{
    setIdentificationString(identifier);
}

/// Return the user-unique identification string for the module, which is intended to be used as
/// an identifier for the module's data as it is being transferred for archive.  This is whatever the
/// programmer provided, and therefore relies on the programmer to ensure that it's unique.
///
/// \return User-unique identifier string for the module

String BluetoothAdapter::LoggerIdentifier(void)
{
    return getIdentificationString();
}

/// Determine whether the interface has been started.
///
/// \return True if the interface has been started.

bool BluetoothAdapter::IsStarted(void)
{
    return isStarted();
}

/// Detemine whether there is a Bluetooth client connected to the adapter interface.
///
/// \return True if there is a client connected, otherwise False

bool BluetoothAdapter::IsConnected(void)
{
  return isConnected();
}

/// Determine whether there is data available at the receiver buffers from the client.
///
/// \return True if data is available, otherwise False

bool BluetoothAdapter::DataAvailable(void)
{
    return dataCount() > 0;
}

/// Fetch the first received string from the BLE client.
///
/// \return Message string from the BLE client, if any

String BluetoothAdapter::ReceivedString(void)
{
    return readBuffer();
}

/// Write a string into the BLE UART service for transfer to the client
///
/// \param data String to send to the client over the BLE radio

void BluetoothAdapter::WriteString(String const& data)
{
    return writeBuffer(data);
}

/// Write a single byte into the BLE UART service for transfer to the client
///
/// \param b    Byte to send to the client over the BLE radio

void BluetoothAdapter::WriteByte(int b)
{
    return writeByte(b);
}
