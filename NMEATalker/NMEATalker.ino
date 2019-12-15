/*! \file nmea_talker.ino
 * \brief Generate faked NMEA0183-formated data on the software serial output line
 * 
 * This generates example NMEA0183 data for a GNSS with depth sounder on a single line,
 * changing the position and depth as a function of time.  The position information is
 * passed out at 1Hz, and the depth data at 0.5-1.0Hz (randomly).  The position and depth
 * are changed over time, with suitable constraints to maintain consistency.
 * 
 * Copyright (c) University of New Hampshire, Center for Coastal and Ocean Mapping &
 * NOAA-UNH Joint Hydrographic Center, 2019.  All Rights Reserved.
 * 
 */

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
