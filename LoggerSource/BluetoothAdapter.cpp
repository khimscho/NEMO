/*! \file BluetoothAdapter.cpp
 *  \brief Implement factory method and adapters for BLE modules
 *
 *  This code implements a simple factory method to generate a module
 *  adapter for the currently configured module, and the interface
 *  implementations for the different modules supported.
 *
 *  Copyright (c) 2019, University of New Hampshire, Center for
 *  Coastal and Ocean Mapping.  All Rights Reserved.
 *
 */

#include <Arduino.h>

#include "BluetoothAdapter.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include <queue>

// If compiling on the ESP32, we use the internal Bluetooth modem, and use
// the flash memory file system for persistent data on the advertising name
// and identification string.

#include "FS.h"
#include "SPIFFS.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// We need some UUIDs to specify the server, and the Rx and Tx
// characteristics.  These are from www.uuidgenerator.net
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_CHAR_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class ServerCallbacks : public BLEServerCallbacks {
public:
    ServerCallbacks(BLEServer *server)
    : m_connected(false), m_wasConnected(false), m_server(server)
    {
    }
    
    void onConnect(BLEServer *server)
    {
        Serial.println("INFO: connected on BLE.");
        m_connected = true;
    }
    
    void onDisconnect(BLEServer *server)
    {
        Serial.println("INFO: disconnected on BLE.");
        m_connected = false;
    }
    
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
    bool      m_connected;
    bool      m_wasConnected;
    BLEServer *m_server;
};

class CharCallbacks : public BLECharacteristicCallbacks {
public:
    CharCallbacks(void)
    {
    }
    
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
    
    int buffered(void) const
    {
        return m_rx.size();
    }
    
    std::string popBuffer(void)
    {
        std::string rx = m_rx.front();
        m_rx.pop();
        return rx;
    }
    
private:
    std::queue<std::string> m_rx;
};

/// \class ESP32Adapter
/// \brief Implement the BLE interface for the ESP32 built-in

class ESP32Adapter : public BluetoothAdapter {
public:
    ESP32Adapter(void)
    : m_started(false), m_server(NULL), m_service(NULL), m_txCharacteristic(NULL),
      m_rxCallbacks(NULL)
    {
        if (!SPIFFS.begin(true)) {
            // "true" here forces a format of the FFS if it isn't already
            // formatted (which will cause it to initially fail).
            Serial.println("ERR: SPIFFS Mount failed.");
        }
        
        // We now need to bring up the BLE server with notification on
        // transmit and call-backs on receive.
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
        m_service->start();
        m_server->getAdvertising()->start();
        Serial.println("INFO: BLE advertising started.");
        
        m_started = true;
    }
    
    ~ESP32Adapter()
    {
        if (m_started) {
            m_server->getAdvertising()->stop();
            m_service->stop();
            delete m_rxCallbacks;
            delete m_serverCallbacks;
            delete m_txCharacteristic;
            delete m_service;
            delete m_server;
        }
    }
    
private:
    bool                m_started;
    BLEServer           *m_server;
    BLEService          *m_service;
    BLECharacteristic   *m_txCharacteristic;
    ServerCallbacks     *m_serverCallbacks;
    CharCallbacks       *m_rxCallbacks;
    
    void setIdentificationString(String const& id)
    {
        File f = SPIFFS.open("/modid.txt", FILE_WRITE);
        if (!f) {
            Serial.println("ERR: module identification string file failed to write.");
            return;
        }
        f.println(id);
    }
    
    String getIdentificationString(void) const
    {
        File f = SPIFFS.open("/modid.txt");
        if (!f) {
            Serial.println("ERR: failed to find identification string file for module.");
            return String("UNKNOWN");
        }
        String rtn = f.readString();
        return rtn;
    }

    void setAdvertisingName(String const& name)
    {
        File f = SPIFFS.open("/modname.txt", FILE_WRITE);
        if (!f) {
            Serial.println("ERR: module advertising name file failed to write.");
            return;
        }
        f.println(name);
    }
    
    String getAdvertisingName(void) const
    {
        File f = SPIFFS.open("/modname.txt");
        if (!f) {
            return String("SB2030");
        }
        String rtn = f.readString();
        return rtn;
    }
    
    bool isConnected(void)
    {
        return m_serverCallbacks->IsConnected();
    }
    
    int dataCount(void)
    {
        return m_rxCallbacks->buffered();
    }
    
    String readBuffer(void)
    {
        return String(m_rxCallbacks->popBuffer().c_str());
    }

    void writeBuffer(String const& data)
    {
        m_txCharacteristic->setValue((uint8_t*)(data.c_str()), data.length());
        m_txCharacteristic->notify();
    }

    void writeByte(uint8_t b)
    {
        m_txCharacteristic->setValue(&b, 1);
        m_txCharacteristic->notify();
    }
};

BluetoothAdapter *BluetoothFactory::Create(void)
{
    return new ESP32Adapter();
}

#else

// All other architectures (typically the Ardunio Due) are assumed to be
// using an external Bluetooth adapter, in this case the Adafruit Bluefruit Friend SPI.

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

const int BLUEFRUIT_SPI_SCLK = 2;
const int BLUEFRUIT_SPI_MISO = 3;
const int BLUEFRUIT_SPI_MOSI = 4;
const int BLUEFRUIT_SPI_CS = 5;
const int BLUEFRUIT_SPI_IRQ = 6;
const int BLUEFRUIT_SPI_RST = 7;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCLK,
                             BLUEFRUIT_SPI_MISO,
                             BLUEFRUIT_SPI_MOSI,
                             BLUEFRUIT_SPI_CS,
                             BLUEFRUIT_SPI_IRQ,
                             BLUEFRUIT_SPI_RST);

/// \class AdafruitAdapter
/// \brief Implement the BLE interface for an Adafruit BLE Friend SPI
class AdafruitAdapter : public BluetoothAdapter {
public:
    AdafruitAdapter(void)
    : m_started(false)
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
    
    ~AdafruitAdapter()
    {
        ble.setMode(BLUEFRUIT_MODE_COMMAND);
        ble.disconnect();
        ble.end();
    }
    
private:
    bool    m_started;      //< BLE interface is started and running
    bool    m_connected;    //< BLE interface has been connected at the other end
    
    const int id_string_start = 0;
    const int max_id_string_length = 32;
    
    const int adv_name_start = id_string_start + 4 + max_id_string_length;
    const int max_adv_name_length = 32;
    
    const int max_nvm_string_length = 32;
    
    void writeNVMString(int offset, String const& s, int max_length) const
    {
        String write_str;
        
        if (s.length() < max_length)
            write_str = s;
        else
            write_str = s.substring(0, max_length);
        
        ble.writeNVM(offset, write_str.length());
        ble.writeNVM(offset+4, (uint8_t const*)(write_str.c_str()), write_str.length());
    }
    
    String readNVMString(int offset) const
    {
        int32_t length;
        uint8_t buffer[max_nvm_string_length+1];
        String rtn;
        
        ble.readNVM(offset, &length);
        ble.readNVM(offset+4, buffer, length);
        buffer[length] = '\0';
        
        return String((const char*)buffer);
    }
    
    void setIdentificationString(String const& id)
    {
        writeNVMString(id_string_start, id, max_id_string_length);
    }
    
    String getIdentificationString(void) const
    {
        return readNVMString(id_string_start);
    }

    void setAdvertisingName(String const& name)
    {
        writeNVMString(adv_name_start, name, max_adv_name_length);
    }
    
    String getAdvertisingName(void) const
    {
        return readNVMString(adv_name_start);
    }
        
    bool isConnected(void)
    {
        bool rc = ble.isConnected();
        if (rc && !m_connected) {
            ble.setMode(BLUEFRUIT_MODE_DATA);
        }
        m_connected = rc;
        return rc;
    }
    
    int dataCount(void)
    {
        return ble.available();
    }
    
    String readBuffer(void)
    {
        String cmd = ble.readStringUntil('\n');
        return cmd;
    }

    void writeBuffer(String const& data)
    {
        ble.println(data);
    }

    void writeByte(uint8_t b)
    {
        ble.write(b);
    }
};

BluetoothAdapter *BluetoothFactory::Create(void)
{
    return new AdafruitAdapter();
}

#endif

// All of the member functions for the BluetoothAdapter are call-throughs
// to the private member methods that the sub-classes have to define.  This
// adds a little to the costs, but it also allows us to instrument the calls
// in a single location, if required.

void BluetoothAdapter::AdvertiseAs(String const& name)
{
    setAdvertisingName(name);
}

void BluetoothAdapter::IdentifyAs(String const& identifier)
{
    setIdentificationString(identifier);
}

String BluetoothAdapter::LoggerIdentifier(void)
{
    return getIdentificationString();
}

bool BluetoothAdapter::IsConnected(void)
{
  return isConnected();
}

bool BluetoothAdapter::DataAvailable(void)
{
    return dataCount() > 0;
}

String BluetoothAdapter::ReceivedString(void)
{
    return readBuffer();
}

void BluetoothAdapter::WriteString(String const& data)
{
    return writeBuffer(data);
}

void BluetoothAdapter::WriteByte(int b)
{
    return writeByte(b);
}
