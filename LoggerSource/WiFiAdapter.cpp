/*! \file WiFiAdapter.h
 *  \brief Interface for the WiFi adapter on the module, and a factory to abstract the object
 *
 * In order to get to a decent speed for data transfer, we need to use the WiFi interface rather
 * than the BLE.  This provides the interface to abstract the details of this away.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping
 * & NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFIAP.h>

#include "WiFiAdapter.h"
#include "ParamStore.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

const int server_port_number = 25443;

/// \class ESP32WiFiAdapter
/// \brief Implementation of the WiFiAdapter base class for the ESP32 module

class ESP32WiFiAdapter : public WiFiAdapter {
public:
    ESP32WiFiAdapter(void)
    : m_paramStore(nullptr), m_server(nullptr)
    {
        if ((m_paramStore = ParamStoreFactory.Create()) == nullptr) {
            return;
        }
    }
    virtual ~ESP32WiFiAdapter(void)
    {
        delete m_paramStore;
        stop();
    }
    
private:
    ParamStore  *m_paramStore;
    WiFiServer  *m_server;
    WiFiClient  m_client;
    
    bool start(void)
    {
        if ((m_server = new WiFiServer(server_port_number)) == nullptr) {
            Serial.println("ERR: failed to start WiFi server.");
            return false;
        }
        WiFi.softAP(get_ssid(), get_password());
        IPAddress server_address = WiFi.softAPIP();
        set_address(server_address.toString());
        m_server->begin();
    }
    void stop(void)
    {
        if (m_client) {
            m_client.stop();
        }
        m_server->end();
        delete m_server;
        m_server = nullptr;
    }
    bool isConnected(void)
    {
        if (!m_client) {
            m_client = m_server->available();
        }
        if (m_client) {
            if (m_client.connected()) {
                return true;
            } else {
                m_client.stop();
                return false;
            }
        }
        return false;
    }
    bool dataCount(void)
    {
        if (isConnected() && m_client.available() > 0) {
            return true;
        } else {
            return false;
        }
    }
    String readBuffer(void)
    {
        String buffer;
        if (!isConnected()) {
            m_client.stop();
            return buffer;
        }
        while (m_client.available() > 0) {
            char c = m_client.read();
            if (c == '\n') {
                return buffer;
            } else {
                buffer += c;
            }
        }
    }
    bool sendLogFile(String const& filename)
    {
        if (!isConnected()) return false;
        SDFile f = SD.open(filename, FILE_READ);
        if (!f) {
            Serial.println("ERR: failed to open file for transfer.");
            return false;
        } else {
            while (f.available()) {
                m_client.write(f.read());
            }
        }
        f.close();
        return true;
    }
    void set_ssid(String const& ssid)
    {
        if (!m_paramStore->SetKey("ssid", ssid)) {
            Serial.println("ERR: failed to set SSID string on module.");
        }
    }
    String get_ssid(void)
    {
        String value;
        if (!m_paramStore->GetKey("ssid", value)) {
            Serial.println("ERR: failed to get SSID string from module.");
            value = "UNKNOWN";
        }
        return value;
    }
    void set_password(String const& password)
    {
        if (!m_paramStore->SetKey("password", password)) {
            Serial.println("ERR: failed to set password on module.");
        }
    }
    String get_password(void)
    {
        String value;
        if (!m_paramStore->GetKey("password", value)) {
            Serial.println("ERR: failed to get password on module.");
            value = "UNKNOWN";
        }
        return value;
    }
    void set_address(IPAddress const& address)
    {
        if (!m_paramStore->SetKey("ipaddress", address)) {
            Serial.println("ERR: failed to set WiFI IP address on module.");
        }
    }
    String get_address(void)
    {
        String value;
        if (!m_paramStore->GetKey("ipaddress", value)) {
            Serial.println("ERR: failed to get WiFi IP address on module.");
            value = "UNKNOWN";
        }
        return value;
    }
};

#endif

bool WiFiAdapter::Startup(void) { return start(); }
void WiFiAdapter::Stop(void) { stop(); }
bool WiFiAdapter::IsConnected(void) { return IsConnected(); }
bool WiFiAdapter::DataAvailable(void) { return dataCount() > 0; }
String WiFiAdapter::ReceivedString(void) { return readBuffer(); }
bool WiFiAdapter::TransferFile(String const& fileaname) { return sendLogFile(filename); }
String WiFiAdapter::GetSSID(void) { return get_ssid(); }
void WiFiAdapter::SetSSID(String const& ssid) { return set_ssid(ssid); }
String WiFiAdapter::GetPassword(void) { return get_password(); }
void WiFiAdapter::SetPassword(String const& password) { set_password(password); }
String WiFiAdapter::GetServerAddress(void) { return get_address(); }

WiFiAdapter *WiFiAdapterFactory::Create(void)
{
    WiFiAdapter *obj;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    obj = new ESP32WiFiAdapter();
#else
#error "No WiFi Adapter supported for hardware."
#endif
    return obj;
}
