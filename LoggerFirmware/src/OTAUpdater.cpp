/*!\file OTAUpdater.cpp
 * \brief Adapter to put ESP32 into "over the air" update mode
 *
 * The ESP32 module allows for the firmware to be updated through the WiFi interface
 * through a particular protocol over a standard port.  This module generates the
 * appropriate server objects to allow this to take place.  Note that this is a
 * terminal condition, since the last thing that happens after OTA update is that
 * the system reboots for the new firmware.  Once the system goes into OTA mode,
 * therefore, it does not return.
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
#include "WiFi.h"
#include "ESPmDNS.h"
#include "WiFiUdp.h"
#include "ArduinoOTA.h"
#include "OTAUpdater.h"
#include "WiFiAdapter.h"

OTAUpdater::OTAUpdater(void)
{
    WiFiAdapter *wifi = WiFiAdapterFactory::Create();
    wifi->Startup();
    ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else
            type = "filesystem";
        Serial.println("Start updating " + type);
        })
        .onEnd([]() {
        Serial.println("\nEnd.");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
        else if (error == OTA_END_ERROR) Serial.println("End failed");
    });

    ArduinoOTA.begin();

    while (false) {
        ArduinoOTA.handle();
    }
}
