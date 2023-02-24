/*! \file Configuration.h
 *  \brief Provide configuration checks for the module
 *
 * There are a number of parameters that need to be set to configure the logger, including
 * things like which logger functionality to bring up (NMEA0183, NMEA2000, Motion Sensor,
 * Power Monitoring, etc.).  These parameters are managed by the ParamStore module, but need
 * to be checked in a number of different locations.  In order to avoid magic strings appearing
 * everywhere in the code, the Config object centralises these requirements.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
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
#include "Configuration.h"

namespace logger {

// Lookup table to translate the Enums in logger::Config::ConfigParam into the strings used to look
// up the keys in ParamStore.  This list has to be in exactly the same order as the elements in the
// Enum, of course, or everything will fall apart.

const String lookup[] = {
    "N1Enable",         ///< Control logging of NMEA0183 data (binary)
    "N2Enable",         ///< Control logging of NMEA2000 data (binary)
    "IMUEnable",        ///< Control logging of motion sensor data (binary)
    "PowMon",           ///< Control whether power monitoring and emergency shutdown is done (binary)
    "MemModule",        ///< Control whether SD/MMC or SPI is used to access the SD card (binary)
    "Bridge",           ///< Control whether to start the UDP->RS-422 bridge on WiFi startup (binary)
    "WebServer",        ///< Control whether to use the web server interface to configure the system
    "modid",            ///< Set the module's Unique ID (string)
    "shipname",         ///< Set the ship's name (string)
    "ap_ssid",          ///< Set the WiFi SSID (string)
    "ap_password",      ///< Set the WiFi password (string)
    "station_ssid",     ///< Set the WiFi SSID (string)
    "station_password", ///< Set the WiFi password (string)
    "ipaddress",        ///< Set the IP address assigned to the WiFI module (string)
    "wifimode",         ///< Set the WiFi mode (access point, station) to start in (string)
    "baud1",            ///< Set the baud rate of NMEA0183 input channel 1
    "baud2",            ///< Set the baud rate of NMEA0183 input channel 2
    "BridgePort",       ///< Set the UDP port for broadcast packets to bridge to RS-422 (string)
    "StationDelay",     ///< Set the timeout between attempts of the webserver joining a client network
    "StationRetries",   ///< Set number of join attempts before the webserver reverts to safe mode
    "StationTimeout",   ///< Set the timeout for any connect attempt
    "WSStatus"          ///< The current status of the configuration webserver
};

/// Default constructor.  This sets up for a dummy parameter store, which is configured
/// on the first instance it's called, to avoid committing the memory until it's actually
/// required.

Config::Config(void)
{
    m_params = nullptr;
}

/// Default destructor.  Simply removes the parameter store interface, if configured.

Config::~Config(void)
{
    delete m_params;
}

/// Extact a configuration string from the parameter store, if it exists (see ParamStore
/// for details).
///
/// \param param    Enum for the key to extract
/// \param value    Reference for where to store the associated value
/// \returns True if the key was successfully extracted, otherwise False

bool Config::GetConfigString(ConfigParam const param, String& value)
{
    String key;
    Lookup(param, key);
    return m_params->GetKey(key, value);
}

/// Set a configuration key's associated value string (see ParamStore for details).
///
/// \param param    Enum for the key to set
/// \param value    String to set as the key's associated value
/// \return True if the key was successfully extracted, otherwise False

bool Config::SetConfigString(ConfigParam const param, String const& value)
{
    String key;
    Lookup(param, key);
    return m_params->SetKey(key, value);
}

/// Extract a binary flag associated with the key provided, if it exists (see
/// ParamStore for details).
///
/// \param param    Enum for the key to extract
/// \param value    Reference for where to store the associated value
/// \return True if the key was extracted successfully, otherwise False

bool Config::GetConfigBinary(ConfigParam const param, bool& value)
{
    String key;
    Lookup(param, key);
    return m_params->GetBinaryKey(key, value);
}

/// Set a binary configuration key to the provided value.
///
/// \param param    Enum for the key to set
/// \param value    Value to set for the associated key
/// \return True if the key was set, otherwise False

bool Config::SetConfigBinary(ConfigParam const param, bool value)
{
    String key;
    Lookup(param, key);
    return m_params->SetBinaryKey(key, value);
}

/// Map from the Enum key for the parameter into the string used to look up the
/// parameter in the ParamStore.
///
/// \param param    Enum to convert into a standard string
/// \param key      Reference for where to store the key's string equivalent

void Config::Lookup(ConfigParam const param, String& key)
{
    if (m_params == nullptr) m_params = ParamStoreFactory::Create();
    key = lookup[static_cast<int>(param)];
}

Config LoggerConfig;    ///< Static parameter to use for lookups (rather than making them on the heap)

}