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

const String lookup[] = {
    "N1Enable",
    "N2Enable",
    "IMUEnable",
    "PowMon",
    "MemModule",
    "BootBLE",
    "modid",
    "modname",
    "ssid",
    "password",
    "ipaddress",
    "wifimode"
    "baud1",
    "baud2"
};

Config::Config(void)
{
    m_params = nullptr;
}

Config::~Config(void)
{
    delete m_params;
}

bool Config::GetConfigString(ConfigParam const param, String& value)
{
    String key;
    Lookup(param, key);
    return m_params->GetKey(key, value);
}

bool Config::SetConfigString(ConfigParam const param, String const& value)
{
    String key;
    Lookup(param, key);
    return m_params->SetKey(key, value);
}

bool Config::GetConfigBinary(ConfigParam const param, bool& value)
{
    String key;
    Lookup(param, key);
    return m_params->GetBinaryKey(key, value);
}

bool Config::SetConfigBinary(ConfigParam const param, bool value)
{
    String key;
    Lookup(param, key);
    return m_params->SetBinaryKey(key, value);
}

void Config::Lookup(ConfigParam const param, String& key)
{
    if (m_params == nullptr) m_params = ParamStoreFactory::Create();
    key = lookup[static_cast<int>(param)];
}

Config LoggerConfig;

}