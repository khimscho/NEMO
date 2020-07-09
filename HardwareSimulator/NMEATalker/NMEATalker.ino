/// @mainpage	NMEATalker
/// @file		NMEATalker.ino
/// @brief		Main sketch
///
/// @details	Main driver for the hardware NMEA simulator
/// @n @a		Developed with [embedXcode+](https://embedXcode.weebly.com)
///
/// @author		Brian Calder
/// @author		CCOM/JHC
/// @date		2020-06-09 11:31
/// @version	1.0
///
/// @see		ReadMe.txt for references
/// @n
/// Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
/// Hydrographic Center, University of New Hampshire.
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights to use,
/// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
/// and to permit persons to whom the Software is furnished to do so, subject to
/// the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
/// OR OTHER DEALINGS IN THE SOFTWARE.

// Core library for code-sense - IDE-based
// !!! Help: http://bit.ly/2AdU7cu
#if defined(WIRING) // Wiring specific
#include "Wiring.h"
#elif defined(MAPLE_IDE) // Maple specific
#include "WProgram.h"
#elif defined(ROBOTIS) // Robotis specific
#include "libpandora_types.h"
#include "pandora.h"
#elif defined(MPIDE) // chipKIT specific
#include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(LITTLEROBOTFRIENDS) // LittleRobotFriends specific
#include "LRF.h"
#elif defined(MICRODUINO) // Microduino specific
#include "Arduino.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(REDBEARLAB) // RedBearLab specific
#include "Arduino.h"
#elif defined(RFDUINO) // RFduino specific
#include "Arduino.h"
#elif defined(SPARK) || defined(PARTICLE) // Particle / Spark specific
#include "application.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

#include "NMEA0183Simulator.h"
#include "NMEA2000Simulator.h"
#include "StatusLED.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
const int rx1_pin = 13; ///< UART port 1 receive pin number (default for this system, not standard)
const int tx1_pin = 32; ///< UART port 1 transmit pin number (default for this system, not standard)
const int rx2_pin = 14; ///< UART port 2 receive pin number (default for this system, not standard)
const int tx2_pin = 33; ///< UART port 2 transmit pin number (default for this system, not standard)
#elif defined(__SAM3X8E__)
// Note that these are the defaults, since there doesn't appear to be a way to adjust on Arduino Due
const int rx1_pin = 19; ///< UART port 1 receive pin
const int tx1_pin = 18; ///< UART port 1 transmit pin
const int rx2_pin = 17; ///< UART port 2 receive pin
const int tx2_pin = 16; ///< UART port 2 transmit pin
#else
#error "No configuration recognised for serial inputs"
#endif

StatusLED *LEDs = nullptr;

void setup()
{
    // Interface at high speed on reporting serial port for debugging
    Serial.begin(115200);

    // NMEA0183 is 4800 baud on Serial 1 and Serial 2
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    Serial1.begin(4800, SERIAL_8N1, rx1_pin, tx1_pin);
    Serial2.begin(4800, SERIAL_8N1, rx2_pin, tx2_pin);
#elif defined(__SAM3X8E__)
    Serial1.begin(4800);
    Serial2.begin(4800);
#endif
    // Configure the CAN bus for NMEA2000, and start background processor
    SetupNMEA2000();
    
    LEDs = new StatusLED();
    LEDs->SetStatus(StatusLED::Status::sNORMAL);
}

void loop()
{
    // Process NMEA0183 messages, if it's the right time
    unsigned long now = millis();
    GenerateZDA(now, LEDs);
    GenerateDepth(now, LEDs);
    GeneratePosition(now, LEDs);

    // Process NMEA2000 messages, if it's the right time
    GenerateNMEA2000();
    
    // Check for flashing of LEDs
    LEDs->ProcessFlash();
}
