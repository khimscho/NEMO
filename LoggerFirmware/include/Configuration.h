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

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <Arduino.h>
#include "ParamStore.h"

namespace logger {

/// \class Config
/// \brief Encapsulate configuration parameter management
class Config {
    public:
        Config(void);
        ~Config(void);

        enum ConfigParam {
            CONFIG_NMEA0183_B = 0,  /* Binary: NMEA0183 logging configured on */
            CONFIG_NMEA2000_B,      /* Binary: NMEA2000 logging configured on */
            CONFIG_MOTION_B,        /* Binary: Motion sensor logging configured on */
            CONFIG_POWMON_B,        /* Binary: Power monitoring configured on */
            CONFIG_SDMMC_B,         /* Binary: SD-MMC (SDIO) interface for log files */
            CONFIG_MODULEID_S,      /* String: User-specified unique identifier for the module */
            CONFIG_BLENAME_S,       /* String: BLE advertising name for the module */
            CONFIG_WIFISSID_S,      /* String: WiFi SSID to use for AP/Station */
            CONFIG_WIFIPSWD_S,      /* String: WiFi password to use */
            CONFIG_WIFIIP_S,        /* String: WiFi IP address assigned */
            CONFIG_WIFIMODE_S,      /* String: WiFi mode (Station, SoftAP) */
        };

        bool GetConfigString(ConfigParam const param, String& value);
        bool SetConfigString(ConfigParam const param, String const& value);

        bool GetConfigBinary(ConfigParam const param, bool& value);
        bool SetConfigBinary(ConfigParam const param, bool value);

    private:
        ParamStore  *m_params;
        void Lookup(ConfigParam const param, String& key);
};

extern Config LoggerConfig;

}

#endif
