/*!\file NMEA2000Simulator.h
 * \brief Provide a simple NMEA2000 simulator engine (from NMEA2000 library examples)
 *
 * This code is an example ("MessageTalker") from the NMEA2000 library source code, which
 * generates sample messages on the CAN bus at appropriate times.  The implementation here,
 * which just splits the code up a little, is custom to this project.
 */

#ifndef __NMEA2000_SIMULATOR_H__
#define __NMEA2000_SIMULATOR_H__

void SetupNMEA2000(void);
void GenerateNMEA2000(void);

#endif
