/*
 * \file serial_number.h
 * \brief Generate a serial number from the Arduino board to use as a logging ID
 * 
 * This pulls a serial number from the board in order to have a unique ID for the logger.
 * In a custom implementation, we'd have to do something equivalent (e.g., a reference number
 * pulled from a PROM/EEPROM).
 * 
 * 2019-08-25.
 */

#ifndef __SERIAL_NUMBER_H__
#define __SERIAL_NUMBER_H__

#include <stdint.h>

uint32_t GetSerialNumber(void);
const char *GetSerialNumberString(void);

#endif
 
