///
/// @mainpage	NMEATalker
///
/// @details	Generate NMEA 0183 and 2000 data from ESP32 test board
/// @n
/// @n
/// @n @a		Developed with [embedXcode+](https://embedXcode.weebly.com)
///
/// @author		Brian Calder
/// @author		CCOM/JHC
/// @date		2020-06-09 11:31
/// @version	1.0.0
///
/// @copyright	(c) Brian Calder, 2020
/// @copyright	BSD 3-term
///
/// @see		ReadMe.txt for references
///


///
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
/// @copyright	(c) Brian Calder, 2020
/// @copyright	BSD 3-term
///
/// @see		ReadMe.txt for references
/// @n
///


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

void setup()
{
    // Interface at high speed on reporting serial port for debugging
    Serial.begin(115200);

    // NMEA0183 is 4800 baud on Serial 1 and Serial 2
    Serial1.begin(4800);
    Serial2.begin(4800);
  
    // Configure the CAN bus for NMEA2000, and start background processor
    SetupNMEA2000();
}

void loop()
{
    // Process NMEA0183 messages, if it's the right time
    unsigned long now = millis();
    GenerateDepth(now);
    GeneratePosition(now);

    // Process NMEA2000 messages, if it's the right time
    GenerateNMEA2000();
}
