/*!\file NMEA0183Simulator.h
 * \brief This module generates a very simple set of NMEA0183 sentences on Serial1 and Serial2
 *
 * This code generates a SDDBT and GPGGA sentence sequence on Serial 1 and Serial 2 (respectively)
 * over time, with the position and depth changing from time to time to demonstrate that the CRC is
 * being computed correctly.
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __NMEA0183_SIMULATOR_H__
#define __NMEA0183_SIMULATOR_H__

void GenerateDepth(unsigned long now);
void GeneratePosition(unsigned long now);

#endif
