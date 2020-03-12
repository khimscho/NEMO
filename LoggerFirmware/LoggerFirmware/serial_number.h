/*!\file serial_number.h
 * \brief Generate a serial number from the Arduino board to use as a logging ID
 *
 * This pulls a serial number from the board in order to have a unique ID for the logger.
 * In a custom implementation, we'd have to do something equivalent (e.g., a reference number
 * pulled from a PROM/EEPROM).
 *
 * Copyright 2019, University of New Hampshire, Center for Coastal and Ocean Mapping
 * and NOAA-UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#ifndef __SERIAL_NUMBER_H__
#define __SERIAL_NUMBER_H__

#include <stdint.h>

/// \brief Get a unique serial number for the board
uint32_t GetSerialNumber(void);

/// \brief Get the unique serial number for the board, and convert to a string representation
const char *GetSerialNumberString(void);

#endif
 
