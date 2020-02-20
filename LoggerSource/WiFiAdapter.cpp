/*! \file WiFiAdapter.h
 *  \brief Interface for the WiFi adapter on the module, and a factory to abstract the object
 *
 * In order to get to a decent speed for data transfer, we need to use the WiFi interface rather
 * than the BLE.  This provides the interface to abstract the details of this away.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping
 * & NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
*/

#include "WiFiAdapater.h"
#include "ParamStore.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

/// \class ESP32WiFiAdapter
/// \brief Implementation of the WiFiAdapter base class for the ESP32 module

class ESP32WiFiAdapter : public WiFiAdapter {
public:
    ESP32WiFiAdapter(void)
    {
        if ((m_paramStore = ParamStoreFactory.Create()) == nullptr) {
            return;
        }
    }
    virtual ~ESP32WiFiAdapter(void)
    {
        delete m_paramStore;
    }
    
private:
    ParamStore  *m_paramStore;
    
    bool start(void)
    {
        
    }
    void stop(void)
    {
        
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
};

#endif

bool WiFiAdapter::Startup(void) { return start(); }
void WiFiAdapter::Stop(void) { stop(); }
String WiFiAdapter::GetSSID(void) { return get_ssid(); }
void WiFiAdapter::SetSSID(String const& ssid) { return set_ssid(ssid); }
String WiFiAdapter::GetPassword(void) { return get_password(); }
void WiFiAdapter::set_password(String const& password) { set_password(password); }

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
