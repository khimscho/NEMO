/*! \file ParamStore.cpp
 *  \brief Implementation of a non-volatile memory parameter store for the logger.
 *
 * In order to provide abstracted storage of configuration parameters, this code generates a
 * factory for the particular hardware implementation, and a sub-class implementation for
 * known hardware.
 *
 * Copyright (c) 2020, University of New Hampshire, Center for Coastal and Ocean Mapping
 * & NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include "ParamStore.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)

#include "FS.h"
#include "SPIFFS.h"

/// \class SPIFSParamStore
/// \brief Implement a key-value pair object in flash storage on the SPIFFS module in the ESP32
class SPIFSParamStore : public ParamStore {
public:
    SPIFSParamStore(void)
    {
        if (!SPIFFS.begin(true)) {
            // "true" here forces a format of the FFS if it isn't already
            // formatted (which will cause it to initially fail).
            Serial.println("ERR: SPIFFS mount failed.");
        }
    }
    virtual ~SPIFSParamStore(void)
    {
        
    }
private:
    bool set_key(String const& key, String const& value)
    {
        File f = SPIFFS.open(key, FILE_WRITE);
        if (!f) {
            Serial.println("ERR: failed to write key to filesystem.");
            return false;
        }
        f.println(value);
        f.close();
        return true;
    }
    bool get_key(String const& key, String& value)
    {
        File f = SPIFFS.open(key, FILE_READ);
        if (!f) {
            Serial.print("ERR: failed to find key \"");
            Serial.print(key);
            Serial.println("\" in filesystem.");
            value = "";
            return false;
        }
        value = f.readString();
        f.close();
        return true;
    }
};

#endif

#if defined(__SAM3X8E__)

/// \class BLEParamStore
/// \brief Implement a key-value pair object in the Adafruit BLE module NVM
class BLEParamStore : public ParamStore {
public:
    BLEParamStore(void)
    {
        
    }
    virtual ~BLEParamStore(void)
    {
        
    }
private:
    /// Maximum length of any string written to the NVM memory on the module
    const int max_nvm_string_length = 28;

    int match_key(String const& key)
    {
        int value = -1;
        if (key == "idstring")
            value = 0;
        else if (key == "adname")
            value = 1;
        else if (key == "ssid")
            value = 2;
        else if (key == "password")
            value = 3;
        else if (key == "ipaddress")
            value = 4;
        return value;
    }
    
    bool set_key(String const& key, String const& value)
    {
        int refnum = match_key(key);
        if (refnum < 0) {
            Serial.println("ERR: key not known.");
            return false;
        }
        String write_str;
        if (value.length() < max_nvm_string_length)
            write_str = value;
        else
            write_str = value.substring(0, max_nvm_string_length);
        int address = (sizeof(int) + max_nvm_string_length) * refnum;
        ble.writeNVM(address, write_str.length());
        ble.writeNVM(address + 4, (uint8_t const*)(write_str.c_str()), write_str.length());
        return true;
    }
    
    bool get_key(String const& key, String& value)
    {
        int refnum = match_key(key);
        if (refnum < 0) {
            Serial.println("ERR: key not known.");
            return false;
        }
        int address = (sizeof(int) + max_nvm_string_length) * refnum;
        int32_t length;
        uint8_t buffer[max_nvm_string_length + 1];
        ble.readNVM(address, &length);
        ble.readNVM(address + 4, buffer, length);
        buffer[length] = '\0';
        
        value = String((const char*)buffer);
        return true;
    }
};

#endif

bool ParamStore::SetKey(String const& key, String const& value)
{
    return set_key(key, value);
}

bool ParamStore::GetKey(String const& key, String& value)
{
    return get_key(key, value);
}

ParamStoreFactory::Create(void)
{
    ParamStore *obj;
    
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    obj = new SPIFSParamStore();
#endif
#if defined(__SAM3X8E__)
    obj = new BLEParamStore();
#endif
    
    return obj;
}
